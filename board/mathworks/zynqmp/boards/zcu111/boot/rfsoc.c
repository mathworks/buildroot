/******************************************************************************
 *
 * Copyright (C) 2018 Xilinx, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Use of the Software is limited solely to applications:
 * (a) running on a Xilinx device, or
 * (b) that interact with a Xilinx device through a bus or interconnect.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the Xilinx shall not be used
 * in advertising or otherwise to promote the sale, use or other dealings in
 * this Software without prior written authorization from Xilinx.
 *
 ******************************************************************************/
/*****************************************************************************/
/**
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   Who    Date     Changes
 * ----- ---    -------- -----------------------------------------------
 * 1.0
 *
 * </pre>
 *
 ******************************************************************************/
/***************************** Include Files *********************************/

#include "cmd_interface.h"
#include "data_interface.h"
#include "error_interface.h"
#include "rfdc_interface.h"
#include "tcp_interface.h"
#include "xrfdc.h"
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <libconfig.h>

//#define DEBUG_RFTOOL 1

/************************** Constant Definitions *****************************/
#define RFDC_DEVICE_ID 0
#define DESIGN_TYPE_REG 0xB0005054
#define MAX_ADC_CHANNELS 8
#define MAX_DAC_CHANNELS 8
#define LMK04208_count 26 
#define LMX2594_A_count 113
#define XIIC_BUS 11 

double adc_sampling_rate_g;
int active_adc_tiles_g ; 
int active_dac_tiles_g; 
config_t cfg, *cf;
char *multiTileSync="off" ,  *versionInfo = NULL ;
char *adcMixerFrequency[MAX_ADC_CHANNELS]={} , *dacMixerFrequency[MAX_ADC_CHANNELS]={} ;
double adcNCOFrequency[8], adcNCOPhase[8],  dacNCOFrequency[8] , dacNCOPhase[8] ; 
extern  LMX2592_A[DAC_MAX][LMX2594_A_count], LMK04208_CKin[LMK04208_MAX][LMK04208_count] ;
int nyquistZoneADC =1,rfdcConfigVersion , isExternalPLLEnabled = 0 ; 
int nyquistZoneDAC =1 ; 
/**************************** Variable Definitions
 * *******************************/

static char rcvBuf[BUF_MAX_LEN] = {
    0}; /* receive buffer of BUF_MAX_LEN character */
static char txBuf[BUF_MAX_LEN] = {0}; /* tx buffer of BUF_MAX_LEN character */

int thread_stat = 0;
extern XRFdc RFdcInst;

int mw_readIntegerParam(config_t *cfg, char *param, int *val)
{
    if (config_lookup_int(cfg, param , val) ==0 )
    {   
        printf("%s is not defined\n",param );
        return -1;
    }
    return 0;
}
int mw_readStringParam(config_t *cfg, char *param,  char **val )
{
    if (config_lookup_string(cfg, param , val) == 0 )
    {   
        printf("%s is not defined\n",param );
        return -1;
    }
    return 0;
}
int mw_readDoubleParam(config_t *cfg, char * param, double  *val)
{
    if (config_lookup_float(cfg, param , val) == 0 )
    {
        printf("%s is not defined\n",param );
        return -1;
    }
    return 0; 
}

int readStringArrayFromConfig(config_t *config , char *param , char **stringData)
{
    const config_setting_t *retries;
    int  count ; 
    retries = config_lookup(config , param );
    count = config_setting_length(retries);
    for (int n = 0; n < count; n++) {
        stringData[n] = config_setting_get_string_elem(retries, n);
        //printf("\t#%d. %s\n", n + 1,  config_setting_get_string_elem(retries, n));
    }
}

void readFloatArrayFromConfig(config_t *cf,  char *paramName , double  *floatData)
{
    const config_setting_t *retries;
    int  count ; 
    retries = config_lookup(cf, paramName );
    count = config_setting_length(retries);
    for (int n = 0; n < count; n++) {
        floatData[n] = config_setting_get_float_elem(retries, n);
        //printf("\t#%d. %f\n", n + 1,config_setting_get_float_elem(retries, n));
    }
}

void readIntegerArrayFromConfig( config_t *cf , char *paramName , int *intData )
{
    const config_setting_t *retries;
    int  count ; 
    retries = config_lookup(cf, paramName);
    count = config_setting_length(retries);
    int n =0;
    for( n = 0 ; n < count  ; n++) {
        intData[n] =  config_setting_get_int_elem(retries, n);
        //printf("\t#%d. %d\n" , n + 1 , config_setting_get_int_elem(retries, n));
    }
}




int mapMixerType( char *type )
{
    if(strcmp(type,"Bypassed") == 0 )
        return XRFDC_MIXER_TYPE_OFF ;
    else if(strcmp(type ,"Coarse") == 0 )
        return XRFDC_MIXER_TYPE_COARSE ;
    else if(strcmp(type ,"Fine") == 0)
        return XRFDC_MIXER_TYPE_FINE; 
}

int mapMixerMode( char *mode)
{
    if(strcmp(mode ,"Real->Real") ==0 )
        return XRFDC_MIXER_MODE_R2R;
    else if(strcmp(mode, "Real->I/Q") ==0 )
        return XRFDC_MIXER_MODE_R2C ;
    else if(strcmp(mode, "I/Q->Real") ==0 )
        return XRFDC_MIXER_MODE_C2R ;
}

int mapInverseSyncFilter( char *mode)
{
    if(strcmp(mode ,"on") == 0 )
        return 1 ;
    else if(strcmp( mode, "off") == 0 )
        return 0 ;
    else
        return -1 ; 
}

unsigned int mapMixerFrequency( char  *freq)
{
    if(strcmp(freq,"Fs/2") ==0)
        return  XRFDC_COARSE_MIX_SAMPLE_FREQ_BY_TWO ;  
    else if(strcmp(freq,"Fs/4") ==0)
        return  XRFDC_COARSE_MIX_SAMPLE_FREQ_BY_FOUR ;  
    else if(strcmp(freq,"-Fs/4") ==0)
        return  XRFDC_COARSE_MIX_MIN_SAMPLE_FREQ_BY_FOUR; 
}

int updateNyquistZone( int nyquestZone)
{
    if ( nyquestZone % 2 == 0 )
        nyquestZone = 1 ;
    else
        nyquestZone = 2 ;

    return nyquestZone ;
}
int calculate_NyquistZone( double PLLSampleRate, double  NCOFrequency)
{
    int NyquistZone = 0;
    if ( NCOFrequency == 0) 
        NyquistZone = 1 ;
    else if (NCOFrequency > 0 )
        NyquistZone = ceil(NCOFrequency/(PLLSampleRate/2));
    else if( NCOFrequency < 0)
        NyquistZone = floor(NCOFrequency/(PLLSampleRate/2));
    return NyquistZone ;
}
int calculate_ADC_Calibration_Mode(double  PLLSampleRate, double eff_NCO_freq)
{
    /*  Selects between different calibration optimization schemes depending on the features of the input signals.
     * Mode 1 is optimal for input frequencies {PLLSampleRate/2(Nyquist) +/-10%}. Otherwise, use Mode 2.
     * Ref: RF Data Converter Interface User Guide UG1309 (v1.2) August 16, 2019 www.xilinx.com
     *      Fig. 15, page 20 */
    int cal_mode = 2;
    if(  abs(PLLSampleRate/2 - abs(eff_NCO_freq)) < ( 0.1*(PLLSampleRate/2)) )
        cal_mode = 1;

    return cal_mode;
}

double calculate_effective_NCO_freq(double PLLSampleRate, double NCOFrequency , short isDAC)
{
    /* This helper function folds NCOFrequency (range -10 GHz to 10 GHz, as defined in PG269) into range +/- PLLSampleRate/2 to create the 'effective NCO Frequency'.

     * this  is done for the purpose of displaying the effective NCO Frequency in the digital domain (-Fs/2 ... Fs/2) as a convenience for the user.
     * Note: xrfdc_mixer.c driver code would automatically perform this check when it receives the absolute Fc in the analog domain within (-10 GHz to 10 GHz)
     * Ref: Xilinx PG269 (v2.1) May 22, 2019, pages 47-48

     * Input arguments:
     *   NCOFrequency :
     *       For the DAC, the 'NCOFrequency' requested by the user indicates the analog domain frequency Fc where a desired image should appear at the output of the DAC
     *       Note: In a practical RF front-end (ex: www.avnet.com\rfsockit), a bandpass filter at the output of the DAC serves as the 're-construction' filter to isolate the desired signal at Fc.
     *
     *       For the ADC, the 'NCOFrequency' requested by the user as -Fc, where +Fc indicates the frequency in the analog domain at the ADC input where lies the signal of interest (-10 GHz to 10 GHz, as defined in PG269)
     *       In other words, the assumption is always that the user wishes to frequency-shift the desired analog input signal at the ADC to baseband in the digital domain.
     *
     *   chanidx: channel index (ones-based)

     * Output results:
     *   eff_NCO_freq: "effective" NCO frequency
     *   For ADC: the mixing frequency in the digital domain folded into range [-Fs/2 ... Fs/2] to shift a digital alias of the original analog signal at Fc, back to DC.

     * n Avnet RFSoC Explorer GUI, effective NCO Frequency is displayed in the semi-circular gauge.

     * Note: Fc determines the ADC calibration Mode (see Xilinx UG1309)

     * excerpt https://github.com/Xilinx/embeddedsw/blob/master/XilinxProcessorIPLib/drivers/rfdc/src/xrfdc_mixer.c (033f4c363031f7a1dc580dc4fd00bf8830239359)
     * reproduce behavior of XRFDC driver */

     double eff_NCO_freq = NCOFrequency;
     int NyquistZone, calibrationMode ;
    if ((eff_NCO_freq < -(PLLSampleRate / 2.0)) || (eff_NCO_freq > (PLLSampleRate / 2.0)))
    {
        while( abs(eff_NCO_freq) > (PLLSampleRate / 2.0))
        {
            if (eff_NCO_freq < -(PLLSampleRate / 2.0))
            {
                eff_NCO_freq =  eff_NCO_freq + PLLSampleRate ;   //Shift towards positive frequencies until reach Nyquist zone -1
            }        
            if (eff_NCO_freq > (PLLSampleRate / 2.0))
            {
                eff_NCO_freq = eff_NCO_freq - PLLSampleRate;    //Shift towards negative frequencies until reach Nyquist zone 1
            }
        }
    }
    printf("Effective NCO Freq is %lf \n",eff_NCO_freq);
    if(isDAC )
    {
        nyquistZoneDAC  = calculate_NyquistZone( PLLSampleRate, NCOFrequency);  // NCOFrequency = desired Fc at DAC analog output
    } else
    {
        /* For the ADC, the calculated effective NCO frequency will be negative if the analog carrier frequency (Fc, aka property 'NCOFrequency') is in 2nd Nyquist,
         * ... such that the alias in -1 Nyquist zone gets pulled towards the + frequencies to baseband
         * Hence the following explanation in Xilinx PG269 (v2.1) May 22, 2019, pages 47-48 for the ADC NCO frequency:
         * The DDC, in general, shifts the carrier from Fc to DC, so with an absolute NCO range (-10 GHz to 10 GHz),
         * setting the NCO to -Fc always shifts the desired signal Fc at DC;
         * setting the NCO to  Fc always shifts the inverse signal to DC, no matter which Nyquist band the signal is located in.
         * When setting the NCO within the effective NCO range (-Fs/2 to Fs/2), to convert the carrier to DC, the NCO frequency should be positive when the signal
         * is at even Nyquist bands, and negative when the signal is at odd Nyquist bands.*/

        nyquistZoneADC  = calculate_NyquistZone(PLLSampleRate, -NCOFrequency);  // -NCOFrequency = NCO Frequency to shift the desired analog signal at the ADC input from carrier Fc (= +NCOFrequency) to DC
        calibrationMode = calculate_ADC_Calibration_Mode(PLLSampleRate, eff_NCO_freq);
    }

  return eff_NCO_freq;
}
XRFdc_Config  *mw_get_sysinfo_from_config_file()
{
    XRFdc_Config *rfdcConfigPtr= NULL;
    const config_setting_t *retries;
    const char *base = NULL;
    cf = &cfg;
    const char *adcMixerType[MAX_ADC_CHANNELS]={} , *adcMixerMode[MAX_ADC_CHANNELS] = {} ;

    char *dacMixerType[MAX_DAC_CHANNELS]= {} , *dacMixerMode[MAX_DAC_CHANNELS] = {};
    double adcSampleRate[8],  dacSampleRate[8],streamClkFreq  ; 
    int  adcDecimationMode[8], adcSamplesPerCycle[8] ,   dacInterpolationMode[8],  dacSamplesPerCycle[8] ;        
    char *dacPLLEnabled=NULL, *adcPLLEnabled=NULL, *dacInvertSyncFilter[8] , *externalPLL = NULL ;  
    int enabledADC[8], enabledADCCnt, enabledDAC[8], enabledDACCnt; 

    active_adc_tiles_g  = 4 ; 
    active_dac_tiles_g  = 2 ; 
    char *adc_mixer_mode , *adc_mixer_type , *dac_mixer_mode ; 
    char *tmp=NULL;
    int count, n ;
    int numADCs ,numDACs  ;
    const config_setting_t *adc_NCO_freq , *adc_NCO_phase ;
    config_init(cf);
    if (!config_read_file(cf, "/mnt/rfdc-configuration.cfg")) {
        fprintf(stderr, "%s:%d - %s\n",
                config_error_file(cf),
                config_error_line(cf),
                config_error_text(cf));
        config_destroy(cf);
        exit(1) ;// return( EXIT_FAILURE);
    }
    rfdcConfigPtr = (XRFdc_Config * )malloc(sizeof(XRFdc_Config)) ;
    if(rfdcConfigPtr == NULL)
    {
        printf("Failed to allocate memory for XRFDC configuration \n");
        exit(1) ;
    }
    int status ; //= mw_readIntegerParam(cf, "numADCs", &numADCs );

    status  = mw_readStringParam(cf, "version" , &versionInfo);
    printf("RFDC config version :%d \n", rfdcConfigVersion);
    readFloatArrayFromConfig(cf , "dacNCOFrequency", dacNCOFrequency );
    readFloatArrayFromConfig(cf , "dacNCOPhase", dacNCOPhase );
    readFloatArrayFromConfig(cf , "adcNCOFrequency", adcNCOFrequency );
    readIntegerArrayFromConfig(cf , "adcNCOPhase", adcNCOPhase );
    if(status < 0 )
    {
        printf("Reading Old Configuraiton \n"); 
        mw_readStringParam(cf, "adcMixerFrequency", adcMixerFrequency);
        mw_readStringParam(cf, "dacMixerFrequency", dacMixerFrequency);
        printf("Reading Old Configuraiton \n"); 
        mw_readIntegerParam(cf, "numADCs", &numADCs );
        mw_readIntegerParam(cf, "adcDecimationMode",&adcDecimationMode[0]);
        mw_readIntegerParam(cf, "dacInterpolationMode",&dacInterpolationMode[0]);
        mw_readIntegerParam(cf, "numDACs" , &numDACs);
        mw_readStringParam(cf, "multiTileSync" , &multiTileSync);
        mw_readStringParam(cf, "adcMixerType", &adcMixerType[0]);
        mw_readStringParam(cf, "adcMixerMode", &adcMixerMode[0]);
        mw_readStringParam(cf, "dacMixerType", &dacMixerType[0]);
        mw_readStringParam(cf , "dacPLLEnabled", &dacPLLEnabled);
        mw_readStringParam(cf , "adcPLLEnabled", &adcPLLEnabled);
        mw_readStringParam(cf, "dacMixerMode", dacMixerMode);
        mw_readStringParam(cf, "dacInvertSyncFilter", &dacInvertSyncFilter[0]);
        mw_readDoubleParam(cf, "adcSampleRate", &adcSampleRate[0]) ;
        mw_readDoubleParam(cf, "dacSampleRate", &dacSampleRate[0]);

        printf("num ADCs  : %d \n ", numADCs);
        printf("num DACs  : %d \n ", numDACs);
        printf("adcMixerFrequency  =  : %s \n ", adcMixerFrequency[0]);
        switch(numADCs)
        {
            case 1 :
                active_adc_tiles_g  = 1 ; 
                active_dac_tiles_g  = 1 ; 
                break;
            case 2 :
                active_adc_tiles_g  = 1 ; 
                active_dac_tiles_g  = 1 ; 
                break;
            case 4:
                active_adc_tiles_g  = 2 ; 
                active_dac_tiles_g  = 1 ; 
                break;
            case 8:
                active_adc_tiles_g  = 4 ; 
                active_dac_tiles_g  = 2 ; 
                break;
        }
        for (int i=1 ;i < MAX_ADC_CHANNELS ; i++)
        {
            adcSampleRate[i] = adcSampleRate[0]; 
            dacInterpolationMode[i] = dacInterpolationMode[0];
            adcSamplesPerCycle[i]  = adcSamplesPerCycle[0];
            adcMixerMode[i] = adcMixerMode[0];
            adcMixerType[i] = adcMixerType[0];
            dacMixerMode[i] = dacMixerMode[0];
            dacMixerType[i] = dacMixerType[0];
            adcMixerFrequency[i] = adcMixerFrequency[0] ;
            dacMixerFrequency[i] = dacMixerFrequency[0];
        }
        for (int tile=0;tile< MAX_ADC_TILE ;tile++)
        {    
            if (tile < active_adc_tiles_g)
            {
                rfdcConfigPtr-> ADCTile_Config[tile].Enable =   1     ;
                rfdcConfigPtr-> ADCTile_Config[tile].SamplingRate = adcSampleRate[0]  ;
                rfdcConfigPtr-> ADCTile_Config[tile].RefClkFreq =  0.0 ;
                rfdcConfigPtr-> ADCTile_Config[tile].FabClkFreq =  0.0 ;
                rfdcConfigPtr-> ADCTile_Config[tile].PLLEnable = strcmp(&adcPLLEnabled[0],"on") ? 0 :1 ; 
                for (int block =0; block < MAX_ADC_PER_TILE ; block++)
                {
                    //Analog config
                    rfdcConfigPtr-> ADCTile_Config[tile].ADCBlock_Analog_Config[block].BlockAvailable = 1 ; 
                    rfdcConfigPtr-> ADCTile_Config[tile].ADCBlock_Analog_Config[block].MixMode= mapMixerMode(adcMixerMode[0]) ; 
                    //Digital config
                    rfdcConfigPtr->ADCTile_Config[tile].ADCBlock_Digital_Config[block].DecimationMode = adcDecimationMode[0]  ; 
                    rfdcConfigPtr-> ADCTile_Config[tile].ADCBlock_Digital_Config[block].MixerType= mapMixerType(adcMixerType[0]) ;
                    rfdcConfigPtr-> ADCTile_Config[tile].ADCBlock_Digital_Config[block].DataWidth = 32 ; 
                }
            }else       {
                //DIsabling all the seetings for these tiles
                rfdcConfigPtr-> ADCTile_Config[tile].Enable =  0;
                rfdcConfigPtr-> ADCTile_Config[tile].PLLEnable = 0 ; 
            }
        }

        for (int tile=0;tile< MAX_DAC_TILE  ;tile++)
        {    
            if( tile < active_dac_tiles_g)
            {
                rfdcConfigPtr-> DACTile_Config[tile].Enable =  1  ;
                rfdcConfigPtr-> DACTile_Config[tile].PLLEnable = strcmp(&dacPLLEnabled[0],"on")? 0 :1 ; 
                rfdcConfigPtr-> DACTile_Config[tile].SamplingRate  = dacSampleRate[0]  ;
                rfdcConfigPtr-> DACTile_Config[tile].RefClkFreq =  0.0 ;
                rfdcConfigPtr-> DACTile_Config[tile].FabClkFreq =  0.0 ;
                for (int block =0; block < MAX_DAC_PER_TILE  ;block++)
                {
                    //Analog config 
                    rfdcConfigPtr->DACTile_Config[tile].DACBlock_Analog_Config[block].BlockAvailable = 1 ; 
                    rfdcConfigPtr-> DACTile_Config[tile].DACBlock_Analog_Config[block].MixMode = mapMixerMode(dacMixerMode[0] ) ;  
                    rfdcConfigPtr->DACTile_Config[tile].DACBlock_Analog_Config[block].InvSyncEnable = mapInverseSyncFilter(dacInvertSyncFilter[0]) ; 
                    //Digital config 
                    rfdcConfigPtr->DACTile_Config[tile].DACBlock_Digital_Config[block].MixerType = mapMixerType(dacMixerType[0])   ;  
                    rfdcConfigPtr->DACTile_Config[tile].DACBlock_Digital_Config[block].DataWidth = 32 ; 
                    rfdcConfigPtr->DACTile_Config[tile].DACBlock_Digital_Config[block].InterploationMode = dacInterpolationMode[0];   
                }
            }else       {
                //DIsabling all the seetings for these tiles
                rfdcConfigPtr-> DACTile_Config[tile].Enable =  0;
                rfdcConfigPtr-> DACTile_Config[tile].PLLEnable = 0 ; 

            }

        }
    }else
    {

        if( strcmp(versionInfo, "1.2") ==0 )
        {
            mw_readStringParam(cf, "externalPLL" , &externalPLL);
            if (strcmp(externalPLL,"on") == 0)
            {
                isExternalPLLEnabled = 1;
                printf("External PLL is Enabled \n ");
            } 
        }
        readFloatArrayFromConfig(cf , "adcSampleRate", adcSampleRate);
        readIntegerArrayFromConfig(cf , "adcDecimationMode", adcDecimationMode);
        readIntegerArrayFromConfig(cf , "adcSamplesPerCycle", adcSamplesPerCycle);
        readStringArrayFromConfig(cf , "adcMixerType",adcMixerType );
        readStringArrayFromConfig(cf , "adcMixerMode",adcMixerMode );
        readStringArrayFromConfig(cf , "adcMixerFrequency",adcMixerFrequency  );

        readFloatArrayFromConfig (cf , "dacSampleRate", dacSampleRate  );
        readIntegerArrayFromConfig( cf,"dacInterpolationMode",dacInterpolationMode  );
        readIntegerArrayFromConfig(cf , "dacSamplesPerCycle",dacSamplesPerCycle );
        readStringArrayFromConfig(cf  , "dacMixerType", dacMixerType );
        readStringArrayFromConfig(cf , "dacMixerMode",  dacMixerMode );
        readStringArrayFromConfig(cf , "dacMixerFrequency", dacMixerFrequency );
        readStringArrayFromConfig(cf , "dacInvertSyncFilter", dacInvertSyncFilter );

        mw_readStringParam(cf , "dacPLLEnabled", &dacPLLEnabled);
        mw_readStringParam(cf , "adcPLLEnabled", &adcPLLEnabled);
        mw_readDoubleParam(cf , "streamClkFreq", &streamClkFreq );
        readIntegerArrayFromConfig(cf , "enabledADC", enabledADC );
        mw_readIntegerParam(cf , "enabledADCCnt", &enabledADCCnt );
        readIntegerArrayFromConfig(cf , "enabledDAC", enabledDAC);
        mw_readIntegerParam(cf , "enabledDACCnt", &enabledDACCnt );
        mw_readStringParam(cf , "multiTileSync", &multiTileSync);

        //printf("Multi tile sync is disabled : num of ADC tiles %d , num of DAC tiles %d \n", active_adc_tiles_g , active_dac_tiles_g);  
        for (int tile=0;tile< MAX_ADC_TILE ;tile++)
        {    
            rfdcConfigPtr-> ADCTile_Config[tile].Enable =   enabledADC[tile*MAX_ADC_PER_TILE ] | enabledADC[tile*MAX_ADC_PER_TILE + 1 ]      ;
            rfdcConfigPtr-> ADCTile_Config[tile].SamplingRate = adcSampleRate[tile]  ;
            rfdcConfigPtr-> ADCTile_Config[tile].RefClkFreq =  0.0 ;
            rfdcConfigPtr-> ADCTile_Config[tile].FabClkFreq =  0.0 ;
            rfdcConfigPtr-> ADCTile_Config[tile].PLLEnable = strcmp(&adcPLLEnabled[tile],"on") ? 0 :1 ; 

            for (int block =0; block < MAX_ADC_PER_TILE ; block++)
            {
                //Analog config
                rfdcConfigPtr-> ADCTile_Config[tile].ADCBlock_Analog_Config[block].BlockAvailable = enabledADC[tile*MAX_ADC_PER_TILE + block ] ; 
                rfdcConfigPtr-> ADCTile_Config[tile].ADCBlock_Analog_Config[block].MixMode= mapMixerMode(adcMixerMode[tile*MAX_ADC_PER_TILE + block]); 
                //Digital config
                rfdcConfigPtr->ADCTile_Config[tile].ADCBlock_Digital_Config[block].DecimationMode = adcDecimationMode[tile*MAX_ADC_PER_TILE + block]  ; 
                rfdcConfigPtr-> ADCTile_Config[tile].ADCBlock_Digital_Config[block].MixerType= mapMixerType(adcMixerType[ tile*MAX_ADC_PER_TILE + block]) ;
                rfdcConfigPtr-> ADCTile_Config[tile].ADCBlock_Digital_Config[block].DataWidth = 32 ; 
            }
        }

        for (int tile=0;tile< MAX_DAC_TILE  ;tile++)
        {    
            rfdcConfigPtr-> DACTile_Config[tile].Enable =  (enabledDAC[tile*MAX_DAC_PER_TILE ]) | (enabledDAC[tile*MAX_DAC_PER_TILE + 1] )  | (enabledDAC[tile*MAX_DAC_PER_TILE + 2 ]) | (enabledDAC[tile*MAX_DAC_PER_TILE +  3 ]   ) ;
            rfdcConfigPtr-> DACTile_Config[tile].PLLEnable = strcmp(&dacPLLEnabled[tile],"on")? 0 :1 ; 
            rfdcConfigPtr-> DACTile_Config[tile].SamplingRate  = dacSampleRate[tile]  ;
            rfdcConfigPtr-> DACTile_Config[tile].RefClkFreq =  0.0 ;
            rfdcConfigPtr-> DACTile_Config[tile].FabClkFreq =  0.0 ;
            for (int block =0; block < MAX_DAC_PER_TILE  ;block++)
            {
                //Analog config 
                rfdcConfigPtr->DACTile_Config[tile].DACBlock_Analog_Config[block].BlockAvailable = enabledDAC[tile*MAX_DAC_PER_TILE + block ] ; 
                rfdcConfigPtr-> DACTile_Config[tile].DACBlock_Analog_Config[block].MixMode = mapMixerMode(dacMixerMode[tile*MAX_DAC_PER_TILE + block ] ) ;  
                rfdcConfigPtr->DACTile_Config[tile].DACBlock_Analog_Config[block].InvSyncEnable = mapInverseSyncFilter(dacInvertSyncFilter[tile*MAX_DAC_PER_TILE + block]) ; 
                //Digital config 
                rfdcConfigPtr->DACTile_Config[tile].DACBlock_Digital_Config[block].MixerType = mapMixerType(dacMixerType[tile*MAX_DAC_PER_TILE + block])   ;  
                rfdcConfigPtr->DACTile_Config[tile].DACBlock_Digital_Config[block].DataWidth = 32 ; 
                rfdcConfigPtr->DACTile_Config[tile].DACBlock_Digital_Config[block].InterploationMode = dacInterpolationMode[tile*MAX_DAC_PER_TILE + block];   
            }
        }

    }
    return rfdcConfigPtr;
}

void  mw_dump_configuration_info(XRFdc_Config *mwConfigPtr)
{
    for (int tile=0;tile< MAX_ADC_TILE ;tile++)
    {    
        printf("\nADC tile %d Configuraiton... ", tile );
        printf("\n\t  Tile Enable: %d ",  mwConfigPtr-> ADCTile_Config[tile].Enable) ;
        printf("\n\t  Samplerate :%f ",mwConfigPtr-> ADCTile_Config[tile].SamplingRate) ; 
        printf("\n\t  PLL enabled :%d ",mwConfigPtr-> ADCTile_Config[tile].PLLEnable);  

        for (int block =0; block < MAX_ADC_PER_TILE ; block++)
        {
            //Analog config
            printf("\n\t #### ADC tile:%d  block:%d Configuration #### " , tile, block ); 
            printf("\n\t\t  Block enable:%d ", mwConfigPtr-> ADCTile_Config[tile].ADCBlock_Analog_Config[block].BlockAvailable);
            printf("\n\t\t  Mixer mode :%d ",mwConfigPtr-> ADCTile_Config[tile].ADCBlock_Analog_Config[block].MixMode) ; 

            //Digital config
            printf("\n\t\t  Decimation mode :%d ", mwConfigPtr->ADCTile_Config[tile].ADCBlock_Digital_Config[block].DecimationMode);
            printf("\n\t\t  MixerType  :%d \n ",  mwConfigPtr-> ADCTile_Config[tile].ADCBlock_Digital_Config[block].MixerType) ; 
        }
    }

    for (int tile=0;tile< MAX_DAC_TILE  ;tile++)
    {    
        printf("\nDAC tile %d Configuraiton... ", tile );
        printf("\n\t DAC Enable:%d  ",mwConfigPtr-> DACTile_Config[tile].Enable);
        printf("\n\t DAC PLL enable:%d  ",mwConfigPtr-> DACTile_Config[tile].PLLEnable);
        printf("\n\t DAC SamplingRate :%f  ",mwConfigPtr-> DACTile_Config[tile].SamplingRate);
        for (int block =0; block < MAX_DAC_PER_TILE  ;block++)
        {
            //Analog config 
            printf("\n\t #### DAC tile:%d  block:%d Configuration #### " , tile, block ); 
            printf("\n\t\t  Block enable:%d ", mwConfigPtr->DACTile_Config[tile].DACBlock_Analog_Config[block].BlockAvailable) ;  
            printf("\n\t\t  Mixer mode :%d ", mwConfigPtr-> DACTile_Config[tile].DACBlock_Analog_Config[block].MixMode) ;   
            printf("\n\t\t  Inverse sync filer enabled :%d ", mwConfigPtr->DACTile_Config[tile].DACBlock_Analog_Config[block].InvSyncEnable);  

            //Digital config 
            printf("\n\t\t  Mixer type:  %d \n",  mwConfigPtr->DACTile_Config[tile].DACBlock_Digital_Config[block].MixerType); 
        }
    }

}

void mw_check_fpga_config_with_ps_config(XRFdc_Config *mw_config_ptr)
{
    int *status;
    XRFdc_BlockStatus BlockStatus;
    int Tile_Id;
    char Response[BUF_MAX_LEN]={0};
    XRFdc_IPStatus ipStatus;

    /* calling this function gets the status of the IP */
    *status = XRFdc_GetIPStatus(&RFdcInst, &ipStatus);
    for (Tile_Id = 0; Tile_Id <= 3; Tile_Id++) {
        sprintf(Response," %d %d %d %d %d %d %d %d %d %d ",
                ipStatus.DACTileStatus[Tile_Id].IsEnabled,ipStatus.DACTileStatus[Tile_Id].BlockStatusMask,
                ipStatus.DACTileStatus[Tile_Id].TileState,ipStatus.DACTileStatus[Tile_Id].PowerUpState,
                ipStatus.DACTileStatus[Tile_Id].PLLState,
                ipStatus.ADCTileStatus[Tile_Id].IsEnabled,ipStatus.ADCTileStatus[Tile_Id].BlockStatusMask,
                ipStatus.ADCTileStatus[Tile_Id].TileState,ipStatus.ADCTileStatus[Tile_Id].PowerUpState,
                ipStatus.ADCTileStatus[Tile_Id].PLLState);
        //strncat (txstrPtr, Response,BUF_MAX_LEN);
    }

    sprintf(Response," %d ", ipStatus.State);
    //strncat (txstrPtr, Response,BUF_MAX_LEN);

    if (*status != SUCCESS) {
        printf("\nXRFdc_GetIPStatus failed\r\n");
    } else {
        printf("    **********XRFdc_GetIPStatus*********\r\n");
        printf("IP Status State: %d\r\n",ipStatus.State);
        for (Tile_Id = 0; Tile_Id <= 3; Tile_Id++) {
            /* DAC */
            printf("Tile: %d DAC Enabled= %d \r\n", Tile_Id,ipStatus.DACTileStatus[Tile_Id].IsEnabled);
            printf("  BlockStatus:  0x%x\r\n",   ipStatus.DACTileStatus[Tile_Id].BlockStatusMask);
            printf("  TileState:    0x%08x\r\n", ipStatus.DACTileStatus[Tile_Id].TileState);
            printf("  PowerUpState: 0x%08x\r\n", ipStatus.DACTileStatus[Tile_Id].PowerUpState);
            printf("  PLLState:     0x%08x\r\n", ipStatus.DACTileStatus[Tile_Id].PLLState);

            /* ADC */
            printf("Tile: %d ADC Enabled= %d \r\n", Tile_Id,ipStatus.ADCTileStatus[Tile_Id].IsEnabled);
            printf("  BlockStatus:  0x%x\r\n", ipStatus.ADCTileStatus[Tile_Id].BlockStatusMask);
            printf("  TileState:    0x%08x\r\n", ipStatus.ADCTileStatus[Tile_Id].TileState);
            printf("  PowerUpState: 0x%08x\r\n", ipStatus.ADCTileStatus[Tile_Id].PowerUpState);
            printf("  PLLState:     0x%08x\r\n", ipStatus.ADCTileStatus[Tile_Id].PLLState);
            printf("    *********************************\r\n");

        }
    }

    for(int Tile_Id =0 ; Tile_Id < MAX_DAC_TILE ;Tile_Id++)
    {
        for(int Block_Id = 0;Block_Id < 4  ;Block_Id++)
        {
            *status = XRFdc_GetBlockStatus(&RFdcInst, DAC, Tile_Id, Block_Id, &BlockStatus);
            if (*status != SUCCESS)
            {
                printf("XRFdc_GetBlockStatus() failed\n\r");
            } else {

                printf("    **********XRFdc_GetBlockStatus***********\r\n");
                printf("    Tile_Id:         %d\r\n", Tile_Id);
                printf("    Block_Id:        %d\r\n", Block_Id);
                printf("    SamplingFreq:                         %f\r\n",  BlockStatus.SamplingFreq);
                printf("    AnalogDataPathStatus:                %d\r\n",  BlockStatus.AnalogDataPathStatus);
                printf("    DigitalDataPathStatus:               %d\r\n", BlockStatus.DigitalDataPathStatus);
                printf("    DataPathClocksStatus:                %d\r\n", BlockStatus.DataPathClocksStatus);
                printf("    IsFIFOFlagsAsserted:                 %d\r\n", BlockStatus.IsFIFOFlagsAsserted);
                printf("    IsFIFOFlagsEnabled:                  %d\r\n", BlockStatus.IsFIFOFlagsEnabled);

            }
        }
    }
}
int getPLLRefClkIndex( double refClock)
{
    double refPLLlist[] = { 102.40625, 204.8,245.76, 409.6,  491.52, 737.28,1474.56, 1966.08,  2048.00,  2457.6, 2949.12,3072.00 , 3194.88, 3276.8, 3686.4, 3932.16,4096, 4423.68,4669.44, 4915.2, 5734.4,5898.24,6144.00 ,6389.76, 6400.00 , 6553.6 } ; 
    int i=0, clkIndex ;
    for ( i=0; i< sizeof(refPLLlist) ; i++)
    {
        if( refPLLlist[i] == refClock )
        {
            clkIndex = i;
            return clkIndex ; 
        }
    }
    return clkIndex ; 
}
/*********************************** Main ************************************/
int main(int argc, char *argv[]) {
    int bufLen =
        BUF_MAX_LEN - 1; /* buffer len must be set to max buffer minus 1 */
    int numCharacters;   /* number of character received per command line */
    int cmdStatus, refClkFound = 0 ;       /* status of the command: XST_SUCCES - ERROR_CMD_UNDEF -
                            ERROR_NUM_ARGS - ERROR_EXECUTE */
    double refADCClkFreq = 245.76, refDACClkFreq= 245.76;  
    int ret, tile, block  ;
    pthread_t thread_id;
    XRFdc_Config *rfdcConfigPtr= NULL ;
    void *base;
    int  status ,freqIndexDAC, freqIndexADC ;
    int skip_memory_map = 1;
    char *adc_mixer_type=NULL,*dac_mixer_type =NULL, *dac_invert_sync=NULL ;
    int adc_mixer_freq_div_coarse=0 , dac_mixer_freq_div_coarse=0;
    const config_setting_t *adc_NCO_freq , *adc_NCO_phase, *dac_NCO_freq , *dac_NCO_phase ;
    double effectNCOFreq = 0.0;
    int isConfiguraitonFromNetwork = 0 ;
    XRFdc_MultiConverter_Sync_Config DAC_Sync_Config;
    XRFdc_MultiConverter_Sync_Config ADC_Sync_Config;
    XRFdc_MultiConverter_Sync_Config sync_config;
    if(argc == 2 )
    {

        if (atoi(argv[1]) ==  1)
        {
            printf(" ip core workflow \n ");
            isConfiguraitonFromNetwork = 1;
        }

    }
    printf("RFDC init .....");
    /* initialize RFDC instance */
    ret = rfdc_inst_init(RFDC_DEVICE_ID);
    if (ret != XRFDC_SUCCESS) {
        printf(": Failed to initialize RFDC\n");
        return -1;
    }else
        printf(": Success \n");

    int value = 0 ; 
    value = XRFdc_ReadReg((&RFdcInst), 0, 16424);
    printf("mts enable reg svalue :%d \n",value);

    printf("setting up DAC fifos.... ");
    ret = config_all_fifo(DAC, FIFO_DIS);
    if (ret != SUCCESS) {
        printf(": Failed to disable DAC FIFO\n");
        return -1;
        ;
    } else
        printf(": Success \n");

    printf("Setting up ADC fifos.... ");
    ret = config_all_fifo(ADC, FIFO_DIS);
    if (ret != SUCCESS) {
        printf(" Failed to disable ADC FIFO\n");
        return -1;
    } else
        printf(": Success \n");
    printf("isExternalPLLEnabled :%d \n  ",isExternalPLLEnabled);
    ret = initRFclock(ZCU111, LMK04208_12M8_3072M_122M88_REVA, DAC_245_76_MHZ, DAC_245_76_MHZ, ADC_245_76_MHZ);
    if (ret != SUCCESS) {
        printf("Configuring LMK output Clock 122.88 MHZ .... : Failed \n");
        return ret;
    } else
        printf("Configuring LMK output Clock 122.88 MHZ .... : Success \n");
    if(strcmp(multiTileSync , "on" ) == 0 )
    {
        ret = LMK04208ClockConfig(XIIC_BUS  , &LMK04208_CKin[LMK04208_12M8_3072M_122M88_REVB]);
        if (ret != SUCCESS) {
            printf("Unable to set LMK clock");
            //goto err;
        }
    } 
    usleep(10000);
    if(isConfiguraitonFromNetwork == 0 )
    {
        rfdcConfigPtr = mw_get_sysinfo_from_config_file();
#if DEBUG_RFTOOL
        mw_dump_configuration_info(rfdcConfigPtr);
#endif
        //mw_check_fpga_config_with_ps_config(rfdcConfigPtr);

        for (tile = 0; tile <  MAX_ADC_CHANNELS/MAX_ADC_PER_TILE    ; tile++)
        {
            if (rfdcConfigPtr-> ADCTile_Config[tile].Enable == 1)
            {
                printf("Setting FIFO setup .... ");
                status   =  XRFdc_SetupFIFO(&RFdcInst, ADC , tile ,  1);
                if (status != SUCCESS)
                { 
                    printf(": Failed \n");
                    return -1;
                } else
                    printf(": Success \n"); 
            }
        }
        for (tile = 0; tile < MAX_DAC_TILE  ; tile++)
        {
            if (rfdcConfigPtr-> DACTile_Config[tile].Enable == 1) 
            {
                printf("Setting FIFO setup .... "); 
                status = XRFdc_SetupFIFO(&RFdcInst, DAC , tile ,  1);
                if (status != SUCCESS)
                { 
                    printf(": Failed \n");
                    return -1;
                } else
                    printf(": Success \n"); 
            }
        }
        usleep(1000);
        if(isExternalPLLEnabled == 1 )
        {
             refClkFound=0;
             for (tile = 0; tile <  MAX_ADC_CHANNELS/MAX_ADC_PER_TILE    ; tile++)
             {
                 if ((rfdcConfigPtr-> ADCTile_Config[tile].Enable == 1) &&  (refClkFound == 0 ) )  
                 {
                   refADCClkFreq = rfdcConfigPtr->ADCTile_Config[tile].SamplingRate ;
                   refClkFound = 1;
                 }
             }
             refClkFound = 0;
             for (tile = 0; tile < MAX_DAC_TILE  ; tile++)
             {
                 if ((rfdcConfigPtr-> DACTile_Config[tile].Enable == 1) && (refClkFound == 0 ) )  
                 {
                   refDACClkFreq = rfdcConfigPtr->DACTile_Config[tile].SamplingRate ;
                   refClkFound = 1;
                 }
             }
            freqIndexADC =  getPLLRefClkIndex( refADCClkFreq);
            freqIndexDAC =  getPLLRefClkIndex( refDACClkFreq);
            printf("configuring LMX external mode: DAC ref clock at index:%d : freq value:%f    \n",freqIndexDAC , refDACClkFreq );
            printf("configuring LMX external mode: ADC ref clock at index:%d : freq value:%f    ",freqIndexADC , refADCClkFreq );

            if(strcmp(multiTileSync , "on" ) == 0 )
                 ret = initRFclock(ZCU111, LMK04208_12M8_3072M_122M88_REVB , freqIndexDAC , freqIndexADC  , freqIndexADC );
            else     
                 ret = initRFclock(ZCU111, LMK04208_12M8_3072M_122M88_REVA, freqIndexDAC , freqIndexADC  , freqIndexADC );
            if (ret != SUCCESS) {
                printf(" Failed \n");
                return ret;
            } else
                printf("Success \n");
        }
        for (tile = 0; tile <  MAX_ADC_CHANNELS/MAX_ADC_PER_TILE    ; tile++)
        {
            if (rfdcConfigPtr-> ADCTile_Config[tile].Enable == 1)
            {
                printf("Setting ADC tile-%d frequency to  %f Mhz  and ADC reference clock Frequency to :%f  ... ", tile, rfdcConfigPtr-> ADCTile_Config[tile].SamplingRate, refADCClkFreq );
                if(isExternalPLLEnabled == 1 )
                    ret = XRFdc_DynamicPLLConfig(&RFdcInst, ADC, tile, 0 , refADCClkFreq , rfdcConfigPtr-> ADCTile_Config[tile].SamplingRate   );
                else
                    ret = XRFdc_DynamicPLLConfig(&RFdcInst, ADC, tile, 1 , 245.76  , rfdcConfigPtr-> ADCTile_Config[tile].SamplingRate   );
                if (ret != SUCCESS) {
                    printf(": Failed \n");
                    return -1;
                }
                else
                    printf(" Success \n");
            }
        }
        for (tile = 0; tile < MAX_DAC_TILE  ; tile++)
        {
            if (rfdcConfigPtr-> DACTile_Config[tile].Enable == 1) 
            {
                printf("Setting DAC tile-%d frequency to  %f MHz and DAC reference clock Frequency to :%f  .... ", tile , rfdcConfigPtr-> DACTile_Config[tile].SamplingRate, refDACClkFreq );
                if(isExternalPLLEnabled == 1 )
                {
                    ret = XRFdc_DynamicPLLConfig(&RFdcInst, DAC, tile , 0 , refDACClkFreq , rfdcConfigPtr-> DACTile_Config[tile].SamplingRate );
                
                }else
                    ret = XRFdc_DynamicPLLConfig(&RFdcInst, DAC, tile , 1 , 245.76 , rfdcConfigPtr-> DACTile_Config[tile].SamplingRate );

                if (ret != SUCCESS) {
                    printf(": Failed \n");
                    return -2;
                }
                else
                    printf(": Success \n");
            }
        }
        if(strcmp(multiTileSync , "on" ) == 0 )
        {
            DAC_Sync_Config.Tiles = 3;
            ADC_Sync_Config.Tiles = 15;
            printf("Disabling multi tile sync \n");
            XRFdc_MTS_Sysref_Config(&RFdcInst, &DAC_Sync_Config, &ADC_Sync_Config, 0 );
            usleep(3000000);
        }
        for (tile = 0; tile <  MAX_ADC_CHANNELS/MAX_ADC_PER_TILE    ; tile++)
        {
            if (rfdcConfigPtr-> ADCTile_Config[tile].Enable == 1)
            {
                for (block = 0 ;  block < MAX_ADC_PER_TILE ; block++ ) 
                {
                    if (rfdcConfigPtr-> ADCTile_Config[tile].ADCBlock_Analog_Config[block].BlockAvailable == 1)
                    {
                        printf("\t Settting ADC decimation factor %d for   tile-%d , block-%d  is.... ", rfdcConfigPtr->ADCTile_Config[tile].ADCBlock_Digital_Config[block].DecimationMode , tile, block );
                        status =  XRFdc_SetDecimationFactor(&RFdcInst, tile , block , rfdcConfigPtr->ADCTile_Config[tile].ADCBlock_Digital_Config[block].DecimationMode );
                        if (status != SUCCESS) {
                            printf(": Failed \n",tile);
                            return -1;
                        }
                        else
                            printf(": Success \n");
                    }
                }
                XRFdc_Mixer_Settings mixer_settings ;
                mixer_settings.MixerMode= XRFDC_MIXER_MODE_R2C ;  
                mixer_settings.FineMixerScale= 1;

                for (block =0; block< MAX_ADC_PER_TILE ; block++)
                {
                    if (rfdcConfigPtr-> ADCTile_Config[tile].ADCBlock_Analog_Config[block].BlockAvailable == 1 ) 
                    {

                        if( rfdcConfigPtr-> ADCTile_Config[tile].ADCBlock_Digital_Config[block].MixerType == XRFDC_MIXER_TYPE_COARSE )
                        {
                            mixer_settings.MixerType = XRFDC_MIXER_TYPE_COARSE ;  
                            mixer_settings.Freq = (rfdcConfigPtr-> ADCTile_Config[tile].SamplingRate) ; 
                            mixer_settings.EventSource= 2 ;   //TODO 
                            mixer_settings.CoarseMixFreq = mapMixerFrequency(adcMixerFrequency[tile*MAX_ADC_PER_TILE  + block])  ;
#if DEBUG_RFTOOL
                            printf("\n    FREQ:              %fMHz\n", mixer_settings.Freq);
                            printf("    PHASE OFFSET:      %f\n", mixer_settings.PhaseOffset);
                            printf("    EVENT SOURCE:      %d\n", mixer_settings.EventSource);
                            printf("    FINE MIXER MODE:   %d\n", mixer_settings.MixerMode);
                            printf("    COARSE MIXER FREQ: %d\n", mixer_settings.CoarseMixFreq);
                            printf("    Mixer Type: %d\n", mixer_settings.MixerType);
                            printf("    FINE MIXER SCALE: %d\n", mixer_settings.FineMixerScale);
#endif
                            printf("\t Settting ADC mixer settings for  tile-%d , block-%d .... ", tile,  block );
                            status =  XRFdc_SetMixerSettings(&RFdcInst,ADC, tile , block  ,&mixer_settings  );
                            if (status != SUCCESS) {
                                printf(": Failed \n");
                                return -1;
                            }
                            else
                                printf(": Success \n");
                            printf("\t Resetting NCP Phase for DAC tile:%d , block:%d %d .... ", tile,  block );
                            status = XRFdc_ResetNCOPhase(&RFdcInst , ADC, tile, block);

                        }else if (rfdcConfigPtr-> ADCTile_Config[tile].ADCBlock_Digital_Config[0].MixerType == XRFDC_MIXER_TYPE_FINE)
                        {

#if DEBUG_RFTOOL
                            printf("\n    FREQ:              %fMHz\n", mixer_settings.Freq);
                            int nZoneADC ; 
                            status = XRFdc_GetNyquistZone(&RFdcInst, ADC , tile, block  , &nZoneADC);
                            if (status != SUCCESS) {
                                printf("XRFdc_GetNyquistZone() failed\n\r");
                            } else {
                                printf("    **********XRFdc_GetNyquistZone***********\r\n");
                                printf("    ADC NyquistZone: Before            %d\r\n", nZoneADC );
                            }
#endif
                            mixer_settings.MixerType = XRFDC_MIXER_TYPE_FINE ;  
                            mixer_settings.EventSource= 2 ;    //TODO  
                            mixer_settings.CoarseMixFreq= 0 ;  
                            mixer_settings.PhaseOffset = adcNCOPhase[tile*MAX_ADC_PER_TILE  + block]  ; 
                            effectNCOFreq = calculate_effective_NCO_freq( rfdcConfigPtr-> ADCTile_Config[tile].SamplingRate ,(  adcNCOFrequency[tile*MAX_ADC_PER_TILE  + block] * 1000 )  , 0 )  ;
                            printf("Setting  ADC NyquistZone: to %d\r\n", nyquistZoneADC );
                            XRFdc_SetNyquistZone(&RFdcInst, ADC , tile , block, nyquistZoneADC);
                            usleep(500000);
                            mixer_settings.Freq =  effectNCOFreq ;      
#if DEBUG_RFTOOL
                            printf("    FREQ:              %fMHz\n", mixer_settings.Freq);
                            printf("    PHASE OFFSET:      %f\n", mixer_settings.PhaseOffset);
                            printf("    EVENT SOURCE:      %d\n", mixer_settings.EventSource);
                            printf("    FINE MIXER MODE:   %d\n", mixer_settings.MixerMode);
                            printf("    COARSE MIXER FREQ: %d\n", mixer_settings.CoarseMixFreq);
                            printf("    Mixer Type: %d\n", mixer_settings.MixerType);
                            printf("    FINE MIXER SCALE: %d\n", mixer_settings.FineMixerScale);
#endif
                            printf("\t Settting ADC mixer settings for  tile-%d , block-%d  .... with Frequency:%lf  ", tile , block, mixer_settings.Freq  );
                            status =  XRFdc_SetMixerSettings(&RFdcInst,ADC, tile , block  ,&mixer_settings  );
                            if (status != SUCCESS) {
                                printf(": Failed \n");
                                return -1;
                            }
                            else
                                printf(": Success \n");
                            printf("\t Resetting NCP Phase for ADC tile:%d , block:%d %d .... ", tile,  block );
                            status = XRFdc_ResetNCOPhase(&RFdcInst , ADC, tile, block);

#if DEBUG_RFTOOL
                            printf("\n    FREQ:              %fMHz\n", mixer_settings.Freq);
                            status = XRFdc_GetNyquistZone(&RFdcInst, ADC , tile, block  , &nyquistZoneADC);
                            if (status != SUCCESS) {
                                printf("XRFdc_GetNyquistZone() failed\n\r");
                            } else {
                                printf("    **********XRFdc_GetNyquistZone***********\r\n");
                                printf("   ADC  NyquistZone: After             %d\r\n", nyquistZoneADC );
                            }
#endif                            
                        }
                    }
                }
            }
        }



        for (tile = 0; tile < MAX_DAC_TILE  ; tile++)
        {
            if (rfdcConfigPtr-> DACTile_Config[tile].Enable == 1) 
            {
                for( block =0; block  < MAX_DAC_PER_TILE  ; block++ )
                {
                    if (rfdcConfigPtr->DACTile_Config[tile].DACBlock_Analog_Config[block].BlockAvailable == 1 )
                    {
                        printf("\t Settting Interpolation  mode:%d  for tile-%d  block-%d  ....",rfdcConfigPtr->DACTile_Config[tile].DACBlock_Digital_Config[block].InterploationMode , tile, block  ); 
                        status = XRFdc_SetInterpolationFactor(&RFdcInst, tile , block  , rfdcConfigPtr->DACTile_Config[tile].DACBlock_Digital_Config[block].InterploationMode ); 
                        if (status != SUCCESS)
                        {
                            printf(": Failed \n");
                            return -1;
                        }else
                            printf(": Success \n"); 
                    }
                }
                XRFdc_Mixer_Settings mixer_settings ;
                mixer_settings.MixerMode= XRFDC_MIXER_MODE_C2R ;  
                mixer_settings.FineMixerScale= 1;  

                for (block =0; block< MAX_DAC_PER_TILE ; block++ )
                {
                    if ( rfdcConfigPtr->DACTile_Config[tile].DACBlock_Analog_Config[block].BlockAvailable == 1 ) 
                    {

                        if( rfdcConfigPtr-> DACTile_Config[tile].DACBlock_Digital_Config[block].MixerType == XRFDC_MIXER_TYPE_COARSE )
                        {

                            mixer_settings.MixerType = XRFDC_MIXER_TYPE_COARSE ;  
                            mixer_settings.Freq =  (rfdcConfigPtr-> DACTile_Config[tile].SamplingRate) ; 
                            mixer_settings.EventSource= 0 ; //TODO 
                            mixer_settings.CoarseMixFreq=  mapMixerFrequency(dacMixerFrequency[tile*MAX_DAC_PER_TILE  + block])  ;  


#if DEBUG_RFTOOL
                            printf("    FREQ:             %fMHz Before(%f) \n", mixer_settings.Freq, rfdcConfigPtr-> DACTile_Config[tile].SamplingRate  );
                            printf("    PHASE OFFSET:      %f\n", mixer_settings.PhaseOffset);
                            printf("    EVENT SOURCE:      %d\n", mixer_settings.EventSource);
                            printf("    FINE MIXER MODE:   %d\n", mixer_settings.MixerMode);
                            printf("    COARSE MIXER FREQ: %d\n", mixer_settings.CoarseMixFreq);
                            printf("    Mixer Type: %d\n", mixer_settings.MixerType);
                            printf("    FINE MIXER SCALE: %d\n", mixer_settings.FineMixerScale);
#endif
                            printf("\t Settting DAC mixer settings for  tile-%d , block-%d .... with coarse Frequency:%d  ",tile, block, mixer_settings.CoarseMixFreq  );
                            status =  XRFdc_SetMixerSettings(&RFdcInst,DAC, tile  , block  ,&mixer_settings  );
                            if (status != SUCCESS) {
                                printf(": Failed \n");
                                return -1; 
                            }
                            else
                                printf(": Success \n");
                        }else if (rfdcConfigPtr-> DACTile_Config[tile].DACBlock_Digital_Config[0].MixerType == XRFDC_MIXER_TYPE_FINE)
                        {
#if DEBUG_RFTOOL
                            int nZoneDAC ; 
                            status = XRFdc_GetNyquistZone(&RFdcInst, DAC , tile, block  , &nZoneDAC);
                            if (status != SUCCESS) {
                                printf("XRFdc_GetNyquistZone() failed\n\r");
                            } else {
                                printf("    **********XRFdc_GetNyquistZone***********\r\n");
                                printf("    DAC NyquistZone: Before            %d\r\n", nZoneDAC );
                            }
#endif
                            mixer_settings.MixerType = XRFDC_MIXER_TYPE_FINE ;  
                            mixer_settings.EventSource= 0 ;   //TODO
                            mixer_settings.CoarseMixFreq= 0 ;  
                            mixer_settings.PhaseOffset = dacNCOPhase [tile*MAX_DAC_PER_TILE  + block] ; 
                            effectNCOFreq = calculate_effective_NCO_freq( rfdcConfigPtr-> DACTile_Config[tile].SamplingRate , ( dacNCOFrequency[tile*MAX_DAC_PER_TILE  + block] *1000) , 1 )  ;
                            mixer_settings.Freq =  effectNCOFreq ; 
                            printf("Setting DAC Nequist zone to :%d \n ", nyquistZoneDAC );
                            XRFdc_SetNyquistZone(&RFdcInst, DAC , tile , block, nyquistZoneDAC);
#if DEBUG_RFTOOL
                            printf("    FREQ:              %fMHz\n", mixer_settings.Freq);
                            printf("    PHASE OFFSET:      %f\n", mixer_settings.PhaseOffset);
                            printf("    EVENT SOURCE:      %d\n", mixer_settings.EventSource);
                            printf("    FINE MIXER MODE:   %d\n", mixer_settings.MixerMode);
                            printf("    COARSE MIXER FREQ: %d\n", mixer_settings.CoarseMixFreq);
                            printf("    Mixer Type: %d\n", mixer_settings.MixerType);
                            printf("    FINE MIXER SCALE: %d\n", mixer_settings.FineMixerScale);
#endif      
                            printf("\t Settting DAC mixer settings for  tile-%d , block-%d .... ", tile , block  );
                            status =  XRFdc_SetMixerSettings(&RFdcInst,DAC, tile , block  ,&mixer_settings  );
                            if (status != SUCCESS) {
                                printf(": Failed \n");
                                return -1;
                            }
                            else
                                printf(": Success \n");
#if DEBUG_RFTOOL

                            status = XRFdc_GetNyquistZone(&RFdcInst, DAC , tile, block  , &nyquistZoneDAC);
                            if (status != SUCCESS) {
                                printf("XRFdc_GetNyquistZone() failed\n\r");
                            } else {
                                printf("    **********XRFdc_GetNyquistZone***********\r\n");
                                printf("    DAC NyquistZone:  After           %d\r\n", nyquistZoneDAC );
                            }
#endif 
                        }
                    }
                }
                for (block =0; block< MAX_DAC_PER_TILE ; block++ )
                {
                    if ((rfdcConfigPtr->DACTile_Config[tile].DACBlock_Analog_Config[block].BlockAvailable == 1)  &&  (strcmp(&rfdcConfigPtr->DACTile_Config[tile].DACBlock_Analog_Config[block].InvSyncEnable , "on" ) == 0 ))
                    {
                        printf("Enabling Inverse sync filter for tile-%d  block-%d ....",tile, block  ); 
                        status = XRFdc_SetInvSincFIR(&RFdcInst, tile , block , 1 );
                        if (status != SUCCESS)
                        {
                            printf(": Failed \n");
                            return -1;
                        }else
                            printf(": Success \n"); 
                    }

                }
            }
        }

        if(strcmp(multiTileSync , "on" ) == 0 )
        {
            printf("Enabling Multi tile sync   ");
            DAC_Sync_Config.Tiles = 3;
            ADC_Sync_Config.Tiles = 15;
            int status = XRFdc_MTS_Sysref_Config(&RFdcInst, &DAC_Sync_Config, &ADC_Sync_Config, 1 );
            if(status != SUCCESS) {
                printf(": Failed (error code :%d \n",status);
                return -1; 
            }
            else
                printf(": Success \n");
            printf("DAC MultiConverter_Syncing ....");  
            XRFdc_MultiConverter_Init(&DAC_Sync_Config, 0 , 0);
            DAC_Sync_Config.Tiles = 3 ;
            DAC_Sync_Config.Target_Latency = -1 ;
            status = XRFdc_MultiConverter_Sync(&RFdcInst, DAC  ,&DAC_Sync_Config);
            if(status != SUCCESS) {
                printf(": Failed (Error code %d) \n", errno);
                return -1; 
            }
            else
                printf(": Success \n");
            sync_config = DAC_Sync_Config;

            XRFdc_MultiConverter_Init(&ADC_Sync_Config, 0 , 0);
            ADC_Sync_Config.Tiles = 15 ;
            ADC_Sync_Config.Target_Latency = -1 ;
            printf("ADC MultiConverter_Syncing ....");  
            status = XRFdc_MultiConverter_Sync(&RFdcInst, ADC ,&ADC_Sync_Config);
            if(status != SUCCESS) {
                printf(": Failed \n");
                return -1; 
            }
            else
                printf(": Success \n");
            sync_config = ADC_Sync_Config;

        }  
        printf("RFDC configuration done successfully \n");
    }else { 
        tcpServerInitialize();
        DataServerInitialize();

        printf("Server Init Done\n");

newConn:
        acceptdataConnection();
        printf("Accepted data connection\n");
        acceptConnection();
        printf("Accepted command connection\n");

        /* clear rcvBuf each time anew command is received and processed */
        memset(rcvBuf, 0, sizeof(rcvBuf));
        /* clear txBuf each time anew command is received and a response returned */
        memset(txBuf, 0, sizeof(txBuf));

        /* mark this thread as active */
        thread_stat = 1;

        //  pthread_create(&thread_id, NULL, datapath_t, NULL);

        while (1) {
            /* get string from io interface (Non blocking) */
            numCharacters = getString(rcvBuf, bufLen);
            /* a string with N character is available */
            if (numCharacters > 0) {
                /* parse and run with error check */
                cmdStatus = cmdParse(rcvBuf, txBuf);
                /* check cmParse status - return an error message or the response */
                if (cmdStatus != SUCCESS) {
                    /* command returned an errors */
                    errorIf(txBuf, cmdStatus);
                } else {
                    /* send response */
                    sendString(txBuf, strlen(txBuf));
                }

                if (strcmp(txBuf, "disconnect") == 0) {

                    thread_stat = 0;
                    shutdown_sock(COMMAND);
                    shutdown_sock(DATA);
                    printf("Closed data and command sockets\n");
                    //  pthread_join(thread_id, NULL);
                    break;
                }
                /* clear rcvBuf each time anew command is received and processed */
                memset(rcvBuf, 0, sizeof(rcvBuf));
                /* clear txBuf each time anew command is received and a response returned
                */
                memset(txBuf, 0, sizeof(txBuf));
            } else {


                //    if (pthread_kill(thread_id, 0))
                //    printf("not able to kill data processing thread\n");

                thread_stat = 0;
                break;
            }
        }
        goto newConn;
    }
    //#endif
    return 0;
}

