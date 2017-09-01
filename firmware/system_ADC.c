/*
 * system_ADC.c
 *
 *  Created on: Feb 7, 2017
 *      Author: brent
 */

#include "system_ADC.h"
#include "logging.h"
#include "system_CAN.h"
#include "analogx_api.h"
#include "settings.h"

#define _LOG_PFX "ADC:         "

#define ADC_GRP1_NUM_CHANNELS   ADC_CHANNELS
#define ADC_GRP1_BUF_DEPTH      1
#define SAMPLE_BUFFER_SIZE ADC_GRP1_NUM_CHANNELS * ADC_GRP1_BUF_DEPTH

/* Scale 12 bits to 5.0v */
#define ADC_SCALING 1.0 / 0.819

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

static uint16_t scale_0_to_5_volts(uint16_t raw_value)
{
    float scaled = raw_value * ADC_SCALING;
    return (uint16_t)scaled;
}

struct ADCSamples * system_adc_sample(void)
{
        adcStartConversion(&ADCD1, &adcgrpcfg1, internal_samples, ADC_GRP1_BUF_DEPTH);

        /* re-map samples */
        adc_samples.raw_samples[0] = internal_samples[3];
        adc_samples.raw_samples[1] = internal_samples[2];
        adc_samples.raw_samples[2] = internal_samples[1];
        adc_samples.raw_samples[3] = internal_samples[0];
        return &adc_samples;
}

void system_adc_worker(void)
{
        while(!chThdShouldTerminateX()) {
                systime_t start = chVTGetSystemTimeX();
                struct ADCSamples * adc_samples = system_adc_sample();

                CANTxFrame analog_sample;
                prepare_can_tx_message(&analog_sample, CAN_IDE_EXT, get_can_base_id() + API_BROADCAST_SENSORS);

                analog_sample.data16[0] = scale_0_to_5_volts(adc_samples->raw_samples[0]);
                analog_sample.data16[1] = scale_0_to_5_volts(adc_samples->raw_samples[1]);
                analog_sample.data16[2] = scale_0_to_5_volts(adc_samples->raw_samples[2]);
                analog_sample.data16[3] = scale_0_to_5_volts(adc_samples->raw_samples[3]);

                canTransmit(&CAND1, CAN_ANY_MAILBOX, &analog_sample, MS2ST(CAN_TRANSMIT_TIMEOUT));

                log_debug("Sample ADC %d, %d, %d, %d\r\n", analog_sample.data16[0], analog_sample.data16[1], analog_sample.data16[2], analog_sample.data16[3]);

                /* Compensate for amount of time needed for sampling and broadcasting CAN message */
                systime_t work_time = chVTGetSystemTimeX() - start;

                chThdSleep(MS2ST(1000 / get_sample_rate()) - work_time);
        }
}

