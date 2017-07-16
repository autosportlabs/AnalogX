/*
 * system_ADC.c
 *
 *  Created on: Feb 7, 2017
 *      Author: brent
 */

#include "system_ADC.h"
#include "logging.h"
#define _LOG_PFX "ADC:         "

#define ADC_GRP1_NUM_CHANNELS   ADC_CHANNELS
#define ADC_GRP1_BUF_DEPTH      1
#define SAMPLE_BUFFER_SIZE ADC_GRP1_NUM_CHANNELS * ADC_GRP1_BUF_DEPTH

static adcsample_t internal_samples[SAMPLE_BUFFER_SIZE] = {0};

static struct ADCSamples adc_samples = {0};
/*
 * ADC streaming callback.
 */
static void adccallback(ADCDriver *adcp, adcsample_t *buffer, size_t n)
{
        (void)adcp;
        (void)buffer;
        (void)n;
        internal_samples[0] = buffer[0];
        //log_info(_LOG_PFX " ADC %i\r\n", samples1[0]);
}

static void adcerrorcallback(ADCDriver *adcp, adcerror_t err)
{
        (void)adcp;
        (void)err;
        //log_info(_LOG_PFX " ADC err\r\n");
}

/*
 * ADC conversion group.
 * Mode:        Continuous, 16 samples of 8 channels, SW triggered.
 * Channels:    IN10, IN11, Sensor, VRef.
 */
static const ADCConversionGroup adcgrpcfg1 = {
        FALSE,
        ADC_GRP1_NUM_CHANNELS,
        adccallback,
        adcerrorcallback,
        ADC_CFGR1_RES_12BIT,             /* CFGR1 */
        ADC_TR(0, 0),                                     /* TR */
        ADC_SMPR_SMP_28P5,                                /* SMPR */
        ADC_CHSELR_CHSEL5 | ADC_CHSELR_CHSEL6 | ADC_CHSELR_CHSEL7 | ADC_CHSELR_CHSEL9
};

void system_adc_init(void)
{
        /*
         * Setting up analog inputs used by the demo.
         */
        /* analog 1 */
        palSetGroupMode(GPIOB, PAL_PORT_BIT(1), 0, PAL_MODE_INPUT_ANALOG);

        /* analog 2 */
        palSetGroupMode(GPIOA, PAL_PORT_BIT(7), 0, PAL_MODE_INPUT_ANALOG);

        /* analog 3 */
        palSetGroupMode(GPIOA, PAL_PORT_BIT(6), 0, PAL_MODE_INPUT_ANALOG);

        /* analog 4 */
        palSetGroupMode(GPIOA, PAL_PORT_BIT(5), 0, PAL_MODE_INPUT_ANALOG);

        adcStart(&ADCD1, NULL);

        //  adcSTM32SetCCR(ADC_CCR_VBATEN | ADC_CCR_TSEN | ADC_CCR_VREFEN);
        /* start continuous conversion */
        log_info("adc init\r\n");
}

struct ADCSamples * system_adc_sample(void)
{
        adcStartConversion(&ADCD1, &adcgrpcfg1, internal_samples, ADC_GRP1_BUF_DEPTH);

        /* re-map samples */
        adc_samples.raw_samples[0] = internal_samples[3];
        adc_samples.raw_samples[1] = internal_samples[2];
        adc_samples.raw_samples[2] = internal_samples[1];
        adc_samples.raw_samples[3] = internal_samples[0];

        log_info("Sample ADC %d %d %d, %d\r\n", adc_samples.raw_samples[0], adc_samples.raw_samples[1], adc_samples.raw_samples[2], adc_samples.raw_samples[3]);

        return &adc_samples;
}

