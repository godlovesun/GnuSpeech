/*  REVISION INFORMATION  *****************************************************
 *
 * Revision 1.8  1995/04/17  19:51:21  len
 * Temporary fix to frication balance.
 *
 * Revision 1.7  1995/03/21  04:52:37  len
 * Now compiles FAT.  Also adjusted mono and stereo output volume to match
 * approximately the output volume of the DSP.
 *
 * Revision 1.6  1995/03/04  05:55:57  len
 * Changed controlRate parameter to a float.
 *
 * Revision 1.5  1995/03/02  04:33:04  len
 * Added amplitude scaling to input of vocal tract and throat, to keep the
 * software TRM in line with the DSP version.
 *
 * Revision 1.4  1994/11/24  05:24:12  len
 * Added Hi/Low output sample rate switch.
 *
 * Revision 1.3  1994/10/20  21:20:19  len
 * Changed nose and mouth aperture filter coefficients, so now specified as
 * Hz values (which scale appropriately as the tube length changes), rather
 * than arbitrary coefficient values (which don't scale).
 *
 * Revision 1.2  1994/08/05  03:12:52  len
 * Resectioned tube so that it more closely conforms the the DRM proportions.
 * Also changed frication injection so now allowed from S3 to S10.
 *
 * Revision 1.1.1.1  1994/07/07  03:48:52  len
 * Initial archived version.
 *

******************************************************************************/


/******************************************************************************
*
*     Program:       tube
*
*     Description:   Software (non-real-time) implementation of the Tube
*                    Resonance Model for speech production.
*
*     Author:        Leonard Manzara
*
*     Date:          July 5th, 1994
*
******************************************************************************/


/*  HEADER FILES  ************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <math.h>
#include <string.h>
#include "main.h"
#include "tube.h"
#include "input.h"
#include "fir.h"
#include "util.h"
#include "structs.h"
#include "ring_buffer.h"
#include "wavetable.h"



/*  LOCAL DEFINES  ***********************************************************/

/*  1 MEANS COMPILE SO THAT INTERPOLATION NOT DONE FOR
    SOME CONTROL RATE PARAMETERS  */
#define MATCH_DSP                 0


/*  OROPHARYNX SCATTERING JUNCTION COEFFICIENTS (BETWEEN EACH REGION)  */
#define C1                        R1     /*  R1-R2 (S1-S2)  */
#define C2                        R2     /*  R2-R3 (S2-S3)  */
#define C3                        R3     /*  R3-R4 (S3-S4)  */
#define C4                        R4     /*  R4-R5 (S5-S6)  */
#define C5                        R5     /*  R5-R6 (S7-S8)  */
#define C6                        R6     /*  R6-R7 (S8-S9)  */
#define C7                        R7     /*  R7-R8 (S9-S10)  */
#define C8                        R8     /*  R8-AIR (S10-AIR)  */
#define TOTAL_COEFFICIENTS        TOTAL_REGIONS

/*  OROPHARYNX SECTIONS  */
#define S1                        0      /*  R1  */
#define S2                        1      /*  R2  */
#define S3                        2      /*  R3  */
#define S4                        3      /*  R4  */
#define S5                        4      /*  R4  */
#define S6                        5      /*  R5  */
#define S7                        6      /*  R5  */
#define S8                        7      /*  R6  */
#define S9                        8      /*  R7  */
#define S10                       9      /*  R8  */
#define TOTAL_SECTIONS            10

/*  NASAL TRACT COEFFICIENTS  */
#define NC1                       N1     /*  N1-N2  */
#define NC2                       N2     /*  N2-N3  */
#define NC3                       N3     /*  N3-N4  */
#define NC4                       N4     /*  N4-N5  */
#define NC5                       N5     /*  N5-N6  */
#define NC6                       N6     /*  N6-AIR  */
#define TOTAL_NASAL_COEFFICIENTS  TOTAL_NASAL_SECTIONS

/*  THREE-WAY JUNCTION ALPHA COEFFICIENTS  */
#define LEFT                      0
#define RIGHT                     1
#define UPPER                     2
#define TOTAL_ALPHA_COEFFICIENTS  3

/*  FRICATION INJECTION COEFFICIENTS  */
#define FC1                       0      /*  S3  */
#define FC2                       1      /*  S4  */
#define FC3                       2      /*  S5  */
#define FC4                       3      /*  S6  */
#define FC5                       4      /*  S7  */
#define FC6                       5      /*  S8  */
#define FC7                       6      /*  S9  */
#define FC8                       7      /*  S10  */
#define TOTAL_FRIC_COEFFICIENTS   8


/*  SCALING CONSTANT FOR INPUT TO VOCAL TRACT & THROAT (MATCHES DSP)  */
//#define VT_SCALE                  0.03125     /*  2^(-5)  */
// this is a temporary fix only, to try to match dsp synthesizer
#define VT_SCALE                  0.125     /*  2^(-3)  */

/*  BI-DIRECTIONAL TRANSMISSION LINE POINTERS  */
#define TOP                       0
#define BOTTOM                    1



/*  DERIVED VALUES  */
int    controlPeriod;
int    sampleRate;
double actualTubeLength;            /*  actual length in cm  */

double dampingFactor;               /*  calculated damping factor  */
double crossmixFactor;              /*  calculated crossmix factor  */

double breathinessFactor;

/*  REFLECTION AND RADIATION FILTER MEMORY  */
double a10, b11, a20, a21, b21;

/*  NASAL REFLECTION AND RADIATION FILTER MEMORY  */
double na10, nb11, na20, na21, nb21;

/*  THROAT LOWPASS FILTER MEMORY, GAIN  */
double tb1, ta0, throatGain;

/*  FRICATION BANDPASS FILTER MEMORY  */
double bpAlpha, bpBeta, bpGamma;

/*  MEMORY FOR TUBE AND TUBE COEFFICIENTS  */
double oropharynx[TOTAL_SECTIONS][2][2];
double oropharynx_coeff[TOTAL_COEFFICIENTS];

double nasal[TOTAL_NASAL_SECTIONS][2][2];
double nasal_coeff[TOTAL_NASAL_COEFFICIENTS];

double alpha[TOTAL_ALPHA_COEFFICIENTS];
int current_ptr = 1;
int prev_ptr = 0;

/*  MEMORY FOR FRICATION TAPS  */
double fricationTap[TOTAL_FRIC_COEFFICIENTS];

/*  VARIABLES FOR INTERPOLATION  */
struct {
    struct _TRMParameters parameters;
    struct _TRMParameters delta;
} current;

TRMSampleRateConverter sampleRateConverter;
TRMRingBuffer *ringBuffer;

/*  GLOBAL FUNCTIONS (LOCAL TO THIS FILE)  ***********************************/

void initializeMouthCoefficients(double coeff);
double reflectionFilter(double input);
double radiationFilter(double input);
void initializeNasalFilterCoefficients(double coeff);
double nasalReflectionFilter(double input);
double nasalRadiationFilter(double input);
void setControlRateParameters(INPUT *previousInput, INPUT *currentInput);
void sampleRateInterpolation(void);
void initializeNasalCavity(struct _TRMInputParameters *inputParameters);
void initializeThroat(struct _TRMInputParameters *inputParameters);
void calculateTubeCoefficients(struct _TRMInputParameters *inputParameters);
void setFricationTaps(void);
void calculateBandpassCoefficients(void);
double vocalTract(double input, double frication);
double throat(double input);
double bandpassFilter(double input);
void initializeConversion(struct _TRMInputParameters *inputParameters);
void resampleBuffer(struct _TRMRingBuffer *aRingBuffer, void *context);
void initializeFilter(void);



/******************************************************************************
*
*       function:       initializeSynthesizer
*
*       purpose:        Initializes all variables so that the synthesis can
*                       be run.
*
*       arguments:      none
*
*       internal
*       functions:      speedOfSound, amplitude, initializeWavetable,
*                       initializeFIR, initializeNasalFilterCoefficients,
*                       initializeNasalCavity, initializeThroat,
*                       initializeConversion
*
*       library
*       functions:      rint, fprintf, tmpfile, rewind
*
******************************************************************************/

int initializeSynthesizer(TRMInputParameters *inputParameters)
{
    double nyquist;

    /*  CALCULATE THE SAMPLE RATE, BASED ON NOMINAL
        TUBE LENGTH AND SPEED OF SOUND  */
    if (inputParameters->length > 0.0) {
        double c = speedOfSound(inputParameters->temperature);
        controlPeriod = rint((c * TOTAL_SECTIONS * 100.0) / (inputParameters->length * inputParameters->controlRate));
        sampleRate = inputParameters->controlRate * controlPeriod;
        actualTubeLength = (c * TOTAL_SECTIONS * 100.0) / sampleRate;
        nyquist = (double)sampleRate / 2.0;
    } else {
        fprintf(stderr, "Illegal tube length.\n");
        return ERROR;
    }

    /*  CALCULATE THE BREATHINESS FACTOR  */
    breathinessFactor = inputParameters->breathiness / 100.0;

    /*  CALCULATE CROSSMIX FACTOR  */
    crossmixFactor = 1.0 / amplitude(inputParameters->mixOffset);

    /*  CALCULATE THE DAMPING FACTOR  */
    dampingFactor = (1.0 - (inputParameters->lossFactor / 100.0));

    /*  INITIALIZE THE WAVE TABLE  */
    initializeWavetable(inputParameters->waveform, inputParameters->tp, inputParameters->tnMin, inputParameters->tnMax);

    /*  INITIALIZE THE FIR FILTER  */
    initializeFIR(FIR_BETA, FIR_GAMMA, FIR_CUTOFF);

    /*  INITIALIZE REFLECTION AND RADIATION FILTER COEFFICIENTS FOR MOUTH  */
    initializeMouthCoefficients((nyquist - inputParameters->mouthCoef) / nyquist);

    /*  INITIALIZE REFLECTION AND RADIATION FILTER COEFFICIENTS FOR NOSE  */
    initializeNasalFilterCoefficients((nyquist - inputParameters->noseCoef) / nyquist);

    /*  INITIALIZE NASAL CAVITY FIXED SCATTERING COEFFICIENTS  */
    initializeNasalCavity(inputParameters);

    /*  INITIALIZE THE THROAT LOWPASS FILTER  */
    initializeThroat(inputParameters);

    /*  INITIALIZE THE SAMPLE RATE CONVERSION ROUTINES  */
    initializeConversion(inputParameters);

    /*  RETURN SUCCESS  */
    return SUCCESS;
}



/******************************************************************************
*
*       function:       initializeMouthCoefficients
*
*       purpose:        Calculates the reflection/radiation filter coefficients
*                       for the mouth, according to the mouth aperture
*                       coefficient.
*
*       arguments:      coeff - mouth aperture coefficient
*
*       internal
*       functions:      none
*
*       library
*       functions:      fabs
*
******************************************************************************/

void initializeMouthCoefficients(double coeff)
{
    b11 = -coeff;
    a10 = 1.0 - fabs(b11);

    a20 = coeff;
    a21 = b21 = -a20;
}



/******************************************************************************
*
*       function:       reflectionFilter
*
*       purpose:        Is a variable, one-pole lowpass filter, whose cutoff
*                       is determined by the mouth aperture coefficient.
*
*       arguments:      input
*
*       internal
*       functions:      none
*
*       library
*       functions:      none
*
******************************************************************************/

double reflectionFilter(double input)
{
    static double reflectionY = 0.0;

    double output = (a10 * input) - (b11 * reflectionY);
    reflectionY = output;
    return output;
}



/******************************************************************************
*
*       function:       radiationFilter
*
*       purpose:        Is a variable, one-zero, one-pole, highpass filter,
*                       whose cutoff point is determined by the mouth aperture
*                       coefficient.
*
*       arguments:      input
*
*       internal
*       functions:      none
*
*       library
*       functions:      none
*
******************************************************************************/

double radiationFilter(double input)
{
    static double radiationX = 0.0, radiationY = 0.0;

    double output = (a20 * input) + (a21 * radiationX) - (b21 * radiationY);
    radiationX = input;
    radiationY = output;
    return output;
}



/******************************************************************************
*
*       function:       initializeNasalFilterCoefficients
*
*       purpose:        Calculates the fixed coefficients for the nasal
*                       reflection/radiation filter pair, according to the
*                       nose aperture coefficient.
*
*       arguments:      coeff - nose aperture coefficient
*
*       internal
*       functions:      none
*
*       library
*       functions:      fabs
*
******************************************************************************/

void initializeNasalFilterCoefficients(double coeff)
{
    nb11 = -coeff;
    na10 = 1.0 - fabs(nb11);

    na20 = coeff;
    na21 = nb21 = -na20;
}



/******************************************************************************
*
*       function:       nasalReflectionFilter
*
*       purpose:        Is a one-pole lowpass filter, used for terminating
*                       the end of the nasal cavity.
*
*       arguments:      input
*
*       internal
*       functions:      none
*
*       library
*       functions:      none
*
******************************************************************************/

double nasalReflectionFilter(double input)
{
    static double nasalReflectionY = 0.0;

    double output = (na10 * input) - (nb11 * nasalReflectionY);
    nasalReflectionY = output;
    return output;
}



/******************************************************************************
*
*       function:       nasalRadiationFilter
*
*       purpose:        Is a one-zero, one-pole highpass filter, used for the
*                       radiation characteristic from the nasal cavity.
*
*       arguments:      input
*
*       internal
*       functions:      none
*
*       library
*       functions:      none
*
******************************************************************************/

double nasalRadiationFilter(double input)
{
    static double nasalRadiationX = 0.0, nasalRadiationY = 0.0;

    double output = (na20 * input) + (na21 * nasalRadiationX) - (nb21 * nasalRadiationY);
    nasalRadiationX = input;
    nasalRadiationY = output;
    return output;
}

/******************************************************************************
*
*       function:       synthesize
*
*       purpose:        Performs the actual synthesis of sound samples.
*
*       arguments:      none
*
*       internal
*       functions:      setControlRateParameters, frequency, amplitude,
*                       calculateTubeCoefficients, noise, noiseFilter,
*                       updateWavetable, oscillator, vocalTract, throat,
*                       dataFill, sampleRateInterpolation
*
*       library
*       functions:      none
*
******************************************************************************/

void synthesize(TRMData *data)
{
    int j;
    double f0, ax, ah1, pulse, lp_noise, pulsed_noise, signal, crossmix;
    INPUT *previousInput, *currentInput;



    /*  CONTROL RATE LOOP  */

    previousInput = data->inputHead;
    currentInput = data->inputHead->next;

    while (currentInput != NULL) {
        /*  SET CONTROL RATE PARAMETERS FROM INPUT TABLES  */
        setControlRateParameters(previousInput, currentInput);


        /*  SAMPLE RATE LOOP  */
        for (j = 0; j < controlPeriod; j++) {

            /*  CONVERT PARAMETERS HERE  */
            f0 = frequency(current.parameters.glotPitch);
            ax = amplitude(current.parameters.glotVol);
            ah1 = amplitude(current.parameters.aspVol);
            calculateTubeCoefficients(&(data->inputParameters));
            setFricationTaps();
            calculateBandpassCoefficients();


            /*  DO SYNTHESIS HERE  */
            /*  CREATE LOW-PASS FILTERED NOISE  */
            lp_noise = noiseFilter(noise());

            /*  UPDATE THE SHAPE OF THE GLOTTAL PULSE, IF NECESSARY  */
            if (data->inputParameters.waveform == PULSE)
                updateWavetable(ax);

            /*  CREATE GLOTTAL PULSE (OR SINE TONE)  */
            pulse = oscillator(f0);

            /*  CREATE PULSED NOISE  */
            pulsed_noise = lp_noise * pulse;

            /*  CREATE NOISY GLOTTAL PULSE  */
            pulse = ax * ((pulse * (1.0 - breathinessFactor)) + (pulsed_noise * breathinessFactor));

            /*  CROSS-MIX PURE NOISE WITH PULSED NOISE  */
            if (data->inputParameters.modulation) {
                crossmix = ax * crossmixFactor;
                crossmix = (crossmix < 1.0) ? crossmix : 1.0;
                signal = (pulsed_noise * crossmix) + (lp_noise * (1.0 - crossmix));
                if (verbose) {
                    printf("\nSignal = %e", signal);
                    fflush(stdout);
                }


            } else
                signal = lp_noise;

            /*  PUT SIGNAL THROUGH VOCAL TRACT  */
            signal = vocalTract(((pulse + (ah1 * signal)) * VT_SCALE),
                                bandpassFilter(signal));


            /*  PUT PULSE THROUGH THROAT  */
            signal += throat(pulse * VT_SCALE);
            if (verbose)
                printf("\nDone throat\n");

            /*  OUTPUT SAMPLE HERE  */
            dataFill(ringBuffer, signal);
            if (verbose)
                printf("\nDone datafil\n");

            /*  DO SAMPLE RATE INTERPOLATION OF CONTROL PARAMETERS  */
            sampleRateInterpolation();
            if (verbose)
                printf("\nDone sample rate interp\n");

        }

        previousInput = currentInput;
        currentInput = currentInput->next;
    }

    /*  BE SURE TO FLUSH SRC BUFFER  */
    flushBuffer(ringBuffer);
}


/******************************************************************************
*
*       function:       setControlRateParameters
*
*       purpose:        Calculates the current table values, and their
*                       associated sample-to-sample delta values.
*
*       arguments:      pos
*
*       internal
*       functions:      glotPitchAt, glotVolAt, aspVolAt, fricVolAt, fricPosAt,
*                       fricCFAt, fricBWAt, radiusAtRegion, velumAt,
*
*       library
*       functions:      none
*
******************************************************************************/

void setControlRateParameters(INPUT *previousInput, INPUT *currentInput)
{
    int i;

    /*  GLOTTAL PITCH  */
    current.parameters.glotPitch = glotPitchAt(previousInput);
    current.delta.glotPitch = (glotPitchAt(currentInput) - current.parameters.glotPitch) / (double)controlPeriod;

    /*  GLOTTAL VOLUME  */
    current.parameters.glotVol = glotVolAt(previousInput);
    current.delta.glotVol = (glotVolAt(currentInput) - current.parameters.glotVol) / (double)controlPeriod;

    /*  ASPIRATION VOLUME  */
    current.parameters.aspVol = aspVolAt(previousInput);
#if MATCH_DSP
    current.delta.aspVol = 0.0;
#else
    current.delta.aspVol = (aspVolAt(currentInput) - current.parameters.aspVol) / (double)controlPeriod;
#endif

    /*  FRICATION VOLUME  */
    current.parameters.fricVol = fricVolAt(previousInput);
#if MATCH_DSP
    current.delta.fricVol = 0.0;
#else
    current.delta.fricVol = (fricVolAt(currentInput) - current.parameters.fricVol) / (double)controlPeriod;
#endif

    /*  FRICATION POSITION  */
    current.parameters.fricPos = fricPosAt(previousInput);
#if MATCH_DSP
    current.delta.fricPos = 0.0;
#else
    current.delta.fricPos = (fricPosAt(currentInput) - current.parameters.fricPos) / (double)controlPeriod;
#endif

    /*  FRICATION CENTER FREQUENCY  */
    current.parameters.fricCF = fricCFAt(previousInput);
#if MATCH_DSP
    current.delta.fricCF = 0.0;
#else
    current.delta.fricCF = (fricCFAt(currentInput) - current.parameters.fricCF) / (double)controlPeriod;
#endif

    /*  FRICATION BANDWIDTH  */
    current.parameters.fricBW = fricBWAt(previousInput);
#if MATCH_DSP
    current.delta.fricBW = 0.0;
#else
    current.delta.fricBW = (fricBWAt(currentInput) - current.parameters.fricBW) / (double)controlPeriod;
#endif

    /*  TUBE REGION RADII  */
    for (i = 0; i < TOTAL_REGIONS; i++) {
        current.parameters.radius[i] = radiusAtRegion(previousInput, i);
        current.delta.radius[i] = (radiusAtRegion(currentInput,i) - current.parameters.radius[i]) / (double)controlPeriod;
    }

    /*  VELUM RADIUS  */
    current.parameters.velum = velumAt(previousInput);
    current.delta.velum = (velumAt(currentInput) - current.parameters.velum) / (double)controlPeriod;
}



/******************************************************************************
*
*       function:       sampleRateInterpolation
*
*       purpose:        Interpolates table values at the sample rate.
*
*       arguments:      none
*
*       internal
*       functions:      none
*
*       library
*       functions:      none
*
******************************************************************************/

void sampleRateInterpolation(void)
{
    int i;

    current.parameters.glotPitch += current.delta.glotPitch;
    current.parameters.glotVol += current.delta.glotVol;
    current.parameters.aspVol += current.delta.aspVol;
    current.parameters.fricVol += current.delta.fricVol;
    current.parameters.fricPos += current.delta.fricPos;
    current.parameters.fricCF += current.delta.fricCF;
    current.parameters.fricBW += current.delta.fricBW;
    for (i = 0; i < TOTAL_REGIONS; i++)
        current.parameters.radius[i] += current.delta.radius[i];
    current.parameters.velum += current.delta.velum;
}



/******************************************************************************
*
*       function:       initializeNasalCavity
*
*       purpose:        Calculates the scattering coefficients for the fixed
*                       sections of the nasal cavity.
*
*       arguments:      none
*
*       internal
*       functions:      none
*
*       library
*       functions:      none
*
******************************************************************************/

void initializeNasalCavity(struct _TRMInputParameters *inputParameters)
{
    int i, j;
    double radA2, radB2;


    /*  CALCULATE COEFFICIENTS FOR INTERNAL FIXED SECTIONS OF NASAL CAVITY  */
    for (i = N2, j = NC2; i < N6; i++, j++) {
        radA2 = inputParameters->noseRadius[i] * inputParameters->noseRadius[i];
        radB2 = inputParameters->noseRadius[i+1] * inputParameters->noseRadius[i+1];
        nasal_coeff[j] = (radA2 - radB2) / (radA2 + radB2);
    }

    /*  CALCULATE THE FIXED COEFFICIENT FOR THE NOSE APERTURE  */
    radA2 = inputParameters->noseRadius[N6] * inputParameters->noseRadius[N6];
    radB2 = inputParameters->apScale * inputParameters->apScale;
    nasal_coeff[NC6] = (radA2 - radB2) / (radA2 + radB2);
}



/******************************************************************************
*
*       function:       initializeThroat
*
*       purpose:        Initializes the throat lowpass filter coefficients
*                       according to the throatCutoff value, and also the
*                       throatGain, according to the throatVol value.
*
*       arguments:      none
*
*       internal
*       functions:      none
*
*       library
*       functions:      fabs
*
******************************************************************************/

void initializeThroat(struct _TRMInputParameters *inputParameters)
{
    ta0 = (inputParameters->throatCutoff * 2.0) / sampleRate;
    tb1 = 1.0 - ta0;

    throatGain = amplitude(inputParameters->throatVol);
}



/******************************************************************************
*
*       function:       calculateTubeCoefficients
*
*       purpose:        Calculates the scattering coefficients for the vocal
*                       tract according to the current radii.  Also calculates
*                       the coefficients for the reflection/radiation filter
*                       pair for the mouth and nose.
*
*       arguments:      none
*
*       internal
*       functions:      none
*
*       library
*       functions:      none
*
******************************************************************************/

void calculateTubeCoefficients(struct _TRMInputParameters *inputParameters)
{
    int i;
    double radA2, radB2, r0_2, r1_2, r2_2, sum;


    /*  CALCULATE COEFFICIENTS FOR THE OROPHARYNX  */
    for (i = 0; i < (TOTAL_REGIONS-1); i++) {
        radA2 = current.parameters.radius[i] * current.parameters.radius[i];
        radB2 = current.parameters.radius[i+1] * current.parameters.radius[i+1];
        oropharynx_coeff[i] = (radA2 - radB2) / (radA2 + radB2);
    }

    /*  CALCULATE THE COEFFICIENT FOR THE MOUTH APERTURE  */
    radA2 = current.parameters.radius[R8] * current.parameters.radius[R8];
    radB2 = inputParameters->apScale * inputParameters->apScale;
    oropharynx_coeff[C8] = (radA2 - radB2) / (radA2 + radB2);

    /*  CALCULATE ALPHA COEFFICIENTS FOR 3-WAY JUNCTION  */
    /*  NOTE:  SINCE JUNCTION IS IN MIDDLE OF REGION 4, r0_2 = r1_2  */
    r0_2 = r1_2 = current.parameters.radius[R4] * current.parameters.radius[R4];
    r2_2 = current.parameters.velum * current.parameters.velum;
    sum = 2.0 / (r0_2 + r1_2 + r2_2);
    alpha[LEFT] = sum * r0_2;
    alpha[RIGHT] = sum * r1_2;
    alpha[UPPER] = sum * r2_2;

    /*  AND 1ST NASAL PASSAGE COEFFICIENT  */
    radA2 = current.parameters.velum * current.parameters.velum;
    radB2 = inputParameters->noseRadius[N2] * inputParameters->noseRadius[N2];
    nasal_coeff[NC1] = (radA2 - radB2) / (radA2 + radB2);
}



/******************************************************************************
*
*       function:       setFricationTaps
*
*       purpose:        Sets the frication taps according to the current
*                       position and amplitude of frication.
*
*       arguments:      none
*
*       internal
*       functions:      none
*
*       library
*       functions:      none
*
******************************************************************************/

void setFricationTaps(void)
{
    int i, integerPart;
    double complement, remainder;
    double fricationAmplitude = amplitude(current.parameters.fricVol);


    /*  CALCULATE POSITION REMAINDER AND COMPLEMENT  */
    integerPart = (int)current.parameters.fricPos;
    complement = current.parameters.fricPos - (double)integerPart;
    remainder = 1.0 - complement;

    /*  SET THE FRICATION TAPS  */
    for (i = FC1; i < TOTAL_FRIC_COEFFICIENTS; i++) {
        if (i == integerPart) {
            fricationTap[i] = remainder * fricationAmplitude;
            if ((i+1) < TOTAL_FRIC_COEFFICIENTS)
                fricationTap[++i] = complement * fricationAmplitude;
        } else
            fricationTap[i] = 0.0;
    }

#if DEBUG
    /*  PRINT OUT  */
    printf("fricationTaps:  ");
    for (i = FC1; i < TOTAL_FRIC_COEFFICIENTS; i++)
        printf("%.6f  ", fricationTap[i]);
    printf("\n");
#endif
}



/******************************************************************************
*
*       function:       calculateBandpassCoefficients
*
*       purpose:        Sets the frication bandpass filter coefficients
*                       according to the current center frequency and
*                       bandwidth.
*
*       arguments:      none
*
*       internal
*       functions:      none
*
*       library
*       functions:      tan, cos
*
******************************************************************************/

void calculateBandpassCoefficients(void)
{
    double tanValue, cosValue;


    tanValue = tan((PI * current.parameters.fricBW) / sampleRate);
    cosValue = cos((2.0 * PI * current.parameters.fricCF) / sampleRate);

    bpBeta = (1.0 - tanValue) / (2.0 * (1.0 + tanValue));
    bpGamma = (0.5 + bpBeta) * cosValue;
    bpAlpha = (0.5 - bpBeta) / 2.0;
}



/******************************************************************************
*
*       function:       vocalTract
*
*       purpose:        Updates the pressure wave throughout the vocal tract,
*                       and returns the summed output of the oral and nasal
*                       cavities.  Also injects frication appropriately.
*
*       arguments:      input, frication
*
*       internal
*       functions:      reflectionFilter, radiationFilter,
*                       nasalReflectionFilter, nasalRadiationFilter
*
*       library
*       functions:      none
*
******************************************************************************/

double vocalTract(double input, double frication)
{
    int i, j, k;
    double delta, output, junctionPressure;


    /*  INCREMENT CURRENT AND PREVIOUS POINTERS  */
    if (++current_ptr > 1)
        current_ptr = 0;
    if (++prev_ptr > 1)
        prev_ptr = 0;

    /*  UPDATE OROPHARYNX  */
    /*  INPUT TO TOP OF TUBE  */

    oropharynx[S1][TOP][current_ptr] = (oropharynx[S1][BOTTOM][prev_ptr] * dampingFactor) + input;

    /*  CALCULATE THE SCATTERING JUNCTIONS FOR S1-S2  */

    delta = oropharynx_coeff[C1] * (oropharynx[S1][TOP][prev_ptr] - oropharynx[S2][BOTTOM][prev_ptr]);
    oropharynx[S2][TOP][current_ptr] = (oropharynx[S1][TOP][prev_ptr] + delta) * dampingFactor;
    oropharynx[S1][BOTTOM][current_ptr] = (oropharynx[S2][BOTTOM][prev_ptr] + delta) * dampingFactor;

    /*  CALCULATE THE SCATTERING JUNCTIONS FOR S2-S3 AND S3-S4  */
    if (verbose)
        printf("\nCalc scattering\n");
    for (i = S2, j = C2, k = FC1; i < S4; i++, j++, k++) {
        delta = oropharynx_coeff[j] * (oropharynx[i][TOP][prev_ptr] - oropharynx[i+1][BOTTOM][prev_ptr]);
        oropharynx[i+1][TOP][current_ptr] =
            ((oropharynx[i][TOP][prev_ptr] + delta) * dampingFactor) +
                (fricationTap[k] * frication);
        oropharynx[i][BOTTOM][current_ptr] = (oropharynx[i+1][BOTTOM][prev_ptr] + delta) * dampingFactor;
    }

    /*  UPDATE 3-WAY JUNCTION BETWEEN THE MIDDLE OF R4 AND NASAL CAVITY  */
    junctionPressure = (alpha[LEFT] * oropharynx[S4][TOP][prev_ptr])+
        (alpha[RIGHT] * oropharynx[S5][BOTTOM][prev_ptr]) +
        (alpha[UPPER] * nasal[VELUM][BOTTOM][prev_ptr]);
    oropharynx[S4][BOTTOM][current_ptr] = (junctionPressure - oropharynx[S4][TOP][prev_ptr]) * dampingFactor;
    oropharynx[S5][TOP][current_ptr] =
        ((junctionPressure - oropharynx[S5][BOTTOM][prev_ptr]) * dampingFactor)
            + (fricationTap[FC3] * frication);
    nasal[VELUM][TOP][current_ptr] = (junctionPressure - nasal[VELUM][BOTTOM][prev_ptr]) * dampingFactor;

    /*  CALCULATE JUNCTION BETWEEN R4 AND R5 (S5-S6)  */
    delta = oropharynx_coeff[C4] * (oropharynx[S5][TOP][prev_ptr] - oropharynx[S6][BOTTOM][prev_ptr]);
    oropharynx[S6][TOP][current_ptr] =
        ((oropharynx[S5][TOP][prev_ptr] + delta) * dampingFactor) +
            (fricationTap[FC4] * frication);
    oropharynx[S5][BOTTOM][current_ptr] = (oropharynx[S6][BOTTOM][prev_ptr] + delta) * dampingFactor;

    /*  CALCULATE JUNCTION INSIDE R5 (S6-S7) (PURE DELAY WITH DAMPING)  */
    oropharynx[S7][TOP][current_ptr] =
        (oropharynx[S6][TOP][prev_ptr] * dampingFactor) +
            (fricationTap[FC5] * frication);
    oropharynx[S6][BOTTOM][current_ptr] = oropharynx[S7][BOTTOM][prev_ptr] * dampingFactor;

    /*  CALCULATE LAST 3 INTERNAL JUNCTIONS (S7-S8, S8-S9, S9-S10)  */
    for (i = S7, j = C5, k = FC6; i < S10; i++, j++, k++) {
        delta = oropharynx_coeff[j] * (oropharynx[i][TOP][prev_ptr] - oropharynx[i+1][BOTTOM][prev_ptr]);
        oropharynx[i+1][TOP][current_ptr] =
            ((oropharynx[i][TOP][prev_ptr] + delta) * dampingFactor) +
                (fricationTap[k] * frication);
        oropharynx[i][BOTTOM][current_ptr] = (oropharynx[i+1][BOTTOM][prev_ptr] + delta) * dampingFactor;
    }

    /*  REFLECTED SIGNAL AT MOUTH GOES THROUGH A LOWPASS FILTER  */
    oropharynx[S10][BOTTOM][current_ptr] =  dampingFactor *
        reflectionFilter(oropharynx_coeff[C8] * oropharynx[S10][TOP][prev_ptr]);

    /*  OUTPUT FROM MOUTH GOES THROUGH A HIGHPASS FILTER  */
    output = radiationFilter((1.0 + oropharynx_coeff[C8]) * oropharynx[S10][TOP][prev_ptr]);


    /*  UPDATE NASAL CAVITY  */
    for (i = VELUM, j = NC1; i < N6; i++, j++) {
        delta = nasal_coeff[j] * (nasal[i][TOP][prev_ptr] - nasal[i+1][BOTTOM][prev_ptr]);
        nasal[i+1][TOP][current_ptr] = (nasal[i][TOP][prev_ptr] + delta) * dampingFactor;
        nasal[i][BOTTOM][current_ptr] = (nasal[i+1][BOTTOM][prev_ptr] + delta) * dampingFactor;
    }

    /*  REFLECTED SIGNAL AT NOSE GOES THROUGH A LOWPASS FILTER  */
    nasal[N6][BOTTOM][current_ptr] = dampingFactor * nasalReflectionFilter(nasal_coeff[NC6] * nasal[N6][TOP][prev_ptr]);

    /*  OUTPUT FROM NOSE GOES THROUGH A HIGHPASS FILTER  */
    output += nasalRadiationFilter((1.0 + nasal_coeff[NC6]) * nasal[N6][TOP][prev_ptr]);

    /*  RETURN SUMMED OUTPUT FROM MOUTH AND NOSE  */
    return(output);
}



/******************************************************************************
*
*       function:       throat
*
*       purpose:        Simulates the radiation of sound through the walls
*                       of the throat.  Note that this form of the filter
*                       uses addition instead of subtraction for the
*                       second term, since tb1 has reversed sign.
*
*       arguments:      input
*
*       internal
*       functions:      none
*
*       library
*       functions:      none
*
******************************************************************************/

double throat(double input)
{
    static double throatY = 0.0;

    double output = (ta0 * input) + (tb1 * throatY);
    throatY = output;
    return (output * throatGain);
}



/******************************************************************************
*
*       function:       bandpassFilter
*
*       purpose:        Frication bandpass filter, with variable center
*                       frequency and bandwidth.
*
*       arguments:      input
*
*       internal
*       functions:      none
*
*       library
*       functions:      none
*
******************************************************************************/

double bandpassFilter(double input)
{
    static double xn1 = 0.0, xn2 = 0.0, yn1 = 0.0, yn2 = 0.0;
    double output;


    output = 2.0 * ((bpAlpha * (input - xn2)) + (bpGamma * yn1) - (bpBeta * yn2));

    xn2 = xn1;
    xn1 = input;
    yn2 = yn1;
    yn1 = output;

    return output;
}



/******************************************************************************
*
*       function:       initializeConversion
*
*       purpose:        Initializes all the sample rate conversion functions.
*
*       arguments:      none
*
*       internal
*       functions:      initializeFilter, initializeBuffer
*
*       library
*       functions:      rint, pow
*
******************************************************************************/

void initializeConversion(struct _TRMInputParameters *inputParameters)
{
    double roundedSampleRateRatio;
    int padSize;

    sampleRateConverter.timeRegister = 0;
    sampleRateConverter.maximumSampleValue = 0.0;
    sampleRateConverter.numberSamples = 0;

    /*  INITIALIZE FILTER IMPULSE RESPONSE  */
    initializeFilter();

    /*  CALCULATE SAMPLE RATE RATIO  */
    sampleRateConverter.sampleRateRatio = (double)inputParameters->outputRate / (double)sampleRate;

    /*  CALCULATE TIME REGISTER INCREMENT  */
    sampleRateConverter.timeRegisterIncrement = (int)rint(pow(2.0, FRACTION_BITS) / sampleRateConverter.sampleRateRatio);

    /*  CALCULATE ROUNDED SAMPLE RATE RATIO  */
    roundedSampleRateRatio = pow(2.0, FRACTION_BITS) / (double)sampleRateConverter.timeRegisterIncrement;

    /*  CALCULATE PHASE OR FILTER INCREMENT  */
    if (sampleRateConverter.sampleRateRatio >= 1.0) {
        sampleRateConverter.filterIncrement = L_RANGE;
    } else {
        sampleRateConverter.phaseIncrement = (unsigned int)rint(sampleRateConverter.sampleRateRatio * (double)FRACTION_RANGE);
    }

    /*  CALCULATE PAD SIZE  */
    padSize = (sampleRateConverter.sampleRateRatio >= 1.0) ? ZERO_CROSSINGS :
        (int)((float)ZERO_CROSSINGS / roundedSampleRateRatio) + 1;

    ringBuffer = createRingBuffer(padSize);
    if (ringBuffer == NULL) {
        fprintf(stderr, "Error: Failed to create ring buffer.\n");
    }

    ringBuffer->context = &sampleRateConverter;
    ringBuffer->callbackFunction = resampleBuffer;

    /*  INITIALIZE THE TEMPORARY OUTPUT FILE  */
    sampleRateConverter.tempFilePtr = tmpfile();
    rewind(sampleRateConverter.tempFilePtr);
}

/******************************************************************************
*
*       function:       initializeFilter
*
*       purpose:        Initializes filter impulse response and impulse delta
*                       values.
*
*       arguments:      none
*
*       internal
*       functions:      none
*
*       library
*       functions:      sin, cos
*
******************************************************************************/

void initializeFilter(void)
{
    double x, IBeta;
    int i;


    /*  INITIALIZE THE FILTER IMPULSE RESPONSE  */
    sampleRateConverter.h[0] = LP_CUTOFF;
    x = PI / (double)L_RANGE;
    for (i = 1; i < FILTER_LENGTH; i++) {
        double y = (double)i * x;
        sampleRateConverter.h[i] = sin(y * LP_CUTOFF) / y;
    }

    /*  APPLY A KAISER WINDOW TO THE IMPULSE RESPONSE  */
    IBeta = 1.0 / Izero(BETA);
    for (i = 0; i < FILTER_LENGTH; i++) {
        double temp = (double)i / FILTER_LENGTH;
        sampleRateConverter.h[i] *= Izero(BETA * sqrt(1.0 - (temp * temp))) * IBeta;
    }

    /*  INITIALIZE THE FILTER IMPULSE RESPONSE DELTA VALUES  */
    for (i = 0; i < FILTER_LIMIT; i++)
        sampleRateConverter.deltaH[i] = sampleRateConverter.h[i+1] - sampleRateConverter.h[i];
    sampleRateConverter.deltaH[FILTER_LIMIT] = 0.0 - sampleRateConverter.h[FILTER_LIMIT];
}



// Converts available portion of the input signal to the new sampling
// rate, and outputs the samples to the sound struct.

void resampleBuffer(struct _TRMRingBuffer *aRingBuffer, void *context)
{
    TRMSampleRateConverter *aConverter = (TRMSampleRateConverter *)context;
    int endPtr;

    //printf(" > resampleBuffer()\n");
    //printf("buffer size: %d\n", BUFFER_SIZE);
    //printf("&sampleRateConverter: %p, aConverter: %p\n", &sampleRateConverter, aConverter);
    //printf("numberSamples before: %ld\n", aConverter->numberSamples);
    //printf("fillPtr: %d, padSize: %d\n", aRingBuffer->fillPtr, aRingBuffer->padSize);

    /*  CALCULATE END POINTER  */
    endPtr = aRingBuffer->fillPtr - aRingBuffer->padSize;
    //printf("endPtr: %d\n", endPtr);
    //printf("emptyPtr: %d\n", aRingBuffer->emptyPtr);

    /*  ADJUST THE END POINTER, IF LESS THAN ZERO  */
    if (endPtr < 0)
        endPtr += BUFFER_SIZE;

    /*  ADJUST THE ENDPOINT, IF LESS THEN THE EMPTY POINTER  */
    if (endPtr < aRingBuffer->emptyPtr)
        endPtr += BUFFER_SIZE;

    /*  UPSAMPLE LOOP (SLIGHTLY MORE EFFICIENT THAN DOWNSAMPLING)  */
    //printf("aConverter->sampleRateRatio: %g\n", aConverter->sampleRateRatio);
    if (aConverter->sampleRateRatio >= 1.0) {
        //printf("Upsampling...\n");
        while (aRingBuffer->emptyPtr < endPtr) {
            int index;
            unsigned int filterIndex;
            double output, interpolation, absoluteSampleValue;

            /*  RESET ACCUMULATOR TO ZERO  */
            output = 0.0;

            /*  CALCULATE INTERPOLATION VALUE (STATIC WHEN UPSAMPLING)  */
            interpolation = (double)mValue(aConverter->timeRegister) / (double)M_RANGE;

            /*  COMPUTE THE LEFT SIDE OF THE FILTER CONVOLUTION  */
            index = aRingBuffer->emptyPtr;
            for (filterIndex = lValue(aConverter->timeRegister);
                 filterIndex < FILTER_LENGTH;
                 RBDecrementIndex(&index), filterIndex += aConverter->filterIncrement) {
                output += aRingBuffer->buffer[index] * (aConverter->h[filterIndex] + aConverter->deltaH[filterIndex] * interpolation);
            }

            /*  ADJUST VALUES FOR RIGHT SIDE CALCULATION  */
            aConverter->timeRegister = ~aConverter->timeRegister;
            interpolation = (double)mValue(aConverter->timeRegister) / (double)M_RANGE;

            /*  COMPUTE THE RIGHT SIDE OF THE FILTER CONVOLUTION  */
            index = aRingBuffer->emptyPtr;
            RBIncrementIndex(&index);
            for (filterIndex = lValue(aConverter->timeRegister);
                 filterIndex < FILTER_LENGTH;
                 RBIncrementIndex(&index), filterIndex += aConverter->filterIncrement) {
                output += aRingBuffer->buffer[index] * (aConverter->h[filterIndex] + aConverter->deltaH[filterIndex] * interpolation);
            }

            /*  RECORD MAXIMUM SAMPLE VALUE  */
            absoluteSampleValue = fabs(output);
            if (absoluteSampleValue > aConverter->maximumSampleValue)
                aConverter->maximumSampleValue = absoluteSampleValue;

            /*  INCREMENT SAMPLE NUMBER  */
            aConverter->numberSamples++;

            /*  OUTPUT THE SAMPLE TO THE TEMPORARY FILE  */
            fwrite((char *)&output, sizeof(output), 1, aConverter->tempFilePtr);

            /*  CHANGE TIME REGISTER BACK TO ORIGINAL FORM  */
            aConverter->timeRegister = ~aConverter->timeRegister;

            /*  INCREMENT THE TIME REGISTER  */
            aConverter->timeRegister += aConverter->timeRegisterIncrement;

            /*  INCREMENT THE EMPTY POINTER, ADJUSTING IT AND END POINTER  */
            aRingBuffer->emptyPtr += nValue(aConverter->timeRegister);

            if (aRingBuffer->emptyPtr >= BUFFER_SIZE) {
                aRingBuffer->emptyPtr -= BUFFER_SIZE;
                endPtr -= BUFFER_SIZE;
            }

            /*  CLEAR N PART OF TIME REGISTER  */
            aConverter->timeRegister &= (~N_MASK);
        }
    } else {
        //printf("Downsampling...\n");
        /*  DOWNSAMPLING CONVERSION LOOP  */
        while (aRingBuffer->emptyPtr < endPtr) {
            int index;
            unsigned int phaseIndex, impulseIndex;
            double absoluteSampleValue, output, impulse;

            /*  RESET ACCUMULATOR TO ZERO  */
            output = 0.0;

            /*  COMPUTE P PRIME  */
            phaseIndex = (unsigned int)rint( ((double)fractionValue(aConverter->timeRegister)) * aConverter->sampleRateRatio);

            /*  COMPUTE THE LEFT SIDE OF THE FILTER CONVOLUTION  */
            index = aRingBuffer->emptyPtr;
            while ((impulseIndex = (phaseIndex >> M_BITS)) < FILTER_LENGTH) {
                impulse = aConverter->h[impulseIndex] + (aConverter->deltaH[impulseIndex] *
                                                                 (((double)mValue(phaseIndex)) / (double)M_RANGE));
                output += (aRingBuffer->buffer[index] * impulse);
                RBDecrementIndex(&index);
                phaseIndex += aConverter->phaseIncrement;
            }

            /*  COMPUTE P PRIME, ADJUSTED FOR RIGHT SIDE  */
            phaseIndex = (unsigned int)rint( ((double)fractionValue(~aConverter->timeRegister)) * aConverter->sampleRateRatio);

            /*  COMPUTE THE RIGHT SIDE OF THE FILTER CONVOLUTION  */
            index = aRingBuffer->emptyPtr;
            RBIncrementIndex(&index);
            while ((impulseIndex = (phaseIndex>>M_BITS)) < FILTER_LENGTH) {
                impulse = aConverter->h[impulseIndex] + (aConverter->deltaH[impulseIndex] *
                                                                 (((double)mValue(phaseIndex)) / (double)M_RANGE));
                output += (aRingBuffer->buffer[index] * impulse);
                RBIncrementIndex(&index);
                phaseIndex += aConverter->phaseIncrement;
            }

            /*  RECORD MAXIMUM SAMPLE VALUE  */
            absoluteSampleValue = fabs(output);
            if (absoluteSampleValue > aConverter->maximumSampleValue)
                aConverter->maximumSampleValue = absoluteSampleValue;

            /*  INCREMENT SAMPLE NUMBER  */
            aConverter->numberSamples++;

            /*  OUTPUT THE SAMPLE TO THE TEMPORARY FILE  */
            fwrite((char *)&output, sizeof(output), 1, aConverter->tempFilePtr);

            /*  INCREMENT THE TIME REGISTER  */
            aConverter->timeRegister += aConverter->timeRegisterIncrement;

            /*  INCREMENT THE EMPTY POINTER, ADJUSTING IT AND END POINTER  */
            aRingBuffer->emptyPtr += nValue(aConverter->timeRegister);
            if (aRingBuffer->emptyPtr >= BUFFER_SIZE) {
                aRingBuffer->emptyPtr -= BUFFER_SIZE;
                endPtr -= BUFFER_SIZE;
            }

            /*  CLEAR N PART OF TIME REGISTER  */
            aConverter->timeRegister &= (~N_MASK);
        }
    }
    //printf("numberSamples after: %ld\n", aConverter->numberSamples);
    //printf("<  resampleBuffer()\n");
}
