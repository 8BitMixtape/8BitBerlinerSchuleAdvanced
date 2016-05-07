/*
    Attiny85/84 Based Sequencer
    original code by Adam Berger
    http://www.instructables.com/id/Attiny-Pocket-Sequencer/

    * Modified by dusjagr and stahl to run on the attiny84 for the 8Step MixTape
      Berliner Schule
    * Refactored by manticore to run with TeenySynth sound engine (fork of
      the_synth by DZL and Illuminat modified to support attiny85/84 + other stuff)
    * New Feature added by manticore:
      - options to add sound intro, enable disable layer, setstep on start, select mode on start
      - changed variable to constant to save space
      - Simple menu to change wave, sequencer, and timing

*/

#include <TeenySynth.h>
#include <util/delay.h>

TeenySynth synth;

#define POT_PIN A3
#define TONE_PIN 6
#define CLOCK_PIN 6
#define GATE_PIN 6
#define NUMBER_OF_STEPS 8
#define POT_THRESHOLD 35
#define INTRO_SONG 0
#define CV_PIN 5
#define TRUE 1
#define FALSE 0

#define ENABLE_LAYER_1 1
#define ENABLE_LAYER_2 1
#define ENABLE_LAYER_3 1
#define ENABLE_SETSTEP_ONSTART 0
#define ENABLE_SELECTMODE_ONSTART 1

#define POT_SCALE_TO_4 8
#define POT_SCALE_TO_8 7
#define POT_SCALE_TO_15 6

#define MODE_EXIT_MENU 1
#define MODE_SELECT_LAYER 2
#define MODE_CHANGE_TIMER 3
#define MODE_CHANGE_WAVE 4

#define MENU_SEL_LAYER 1
#define MENU_SEL_STEP 2
#define MENU_SEL_NOTES 3
#define MENU_EXIT_SEL 4
#define MENU_EXIT_THRESHOLD 980

const uint8_t ledPinMapping[8] = {9, 10, 8, 7, 0, 1, 2, 4};

int stepLayer[3][NUMBER_OF_STEPS] =
{
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}
};

uint8_t offsetLayer[3] = {50,30,0};

//for intro song

#if INTRO_SONG == 1
int melody[] =
{
    NOTE_D4, 0, NOTE_F4, NOTE_D4, 0, NOTE_D4, NOTE_G4, NOTE_D4, NOTE_C4,
    NOTE_D4, 0, NOTE_A4, NOTE_D4, 0, NOTE_D4, NOTE_AS4, NOTE_A4, NOTE_F4,
    NOTE_D4, NOTE_A4, NOTE_D5, NOTE_D4, NOTE_C4, 0, NOTE_C4, NOTE_A3, NOTE_E4, NOTE_D4,
    0
};
#endif

uint8_t beat_step_num = 0; // current beat step index 0..1023
unsigned long lastTime = 0; // variable to store the last time we sent a chord
unsigned int pot_value = 0; //variable to store potentio value
uint8_t prev_pressed = 0;
unsigned int beat_tempo = 500;
int raw_pot_value = 0;

static inline void setupSound()
{
    synth.begin();
    synth.setupVoice(0,SQUARE,62,ENVELOPE0,70,64);
    synth.setupVoice(1,RAMP,64,ENVELOPE2,110,64);
    synth.setupVoice(2,NOISE,60,ENVELOPE1,40,64);
    synth.setupVoice(3,NOISE,67,ENVELOPE0,60,64);
}

static inline void soundTrigger(uint8_t voice,int value, uint8_t offsetFreq)
{
    if (value > 0) synth.mTrigger(voice, value + offsetFreq);
}

uint8_t update_pot_value() //return 0 if pressed
{
    raw_pot_value = analogRead(POT_PIN);

    if (raw_pot_value > POT_THRESHOLD)
    {
        pot_value = raw_pot_value - POT_THRESHOLD; //normalized
        return 0;
    }else{
        return 1; // below threshold = pressed
    }
}


static inline void stepToLED(uint8_t beat_step_num)
{
    for (int i = 0; i <= beat_step_num; i++)
        {
            digitalWrite(ledPinMapping[i], HIGH);
        };

    for (int i = beat_step_num; i < NUMBER_OF_STEPS; i++)
        {
            digitalWrite(ledPinMapping[i], LOW);
        }
}


void potToLED(int _pot_value, uint8_t beat_step_index)
{
    uint8_t scaled = (_pot_value) >> POT_SCALE_TO_8;

    for (int i = 0; i <= scaled; i++)  // bit shift right >> 7 for scaling pot value from 0..1023 to 0..7
        {
            digitalWrite(ledPinMapping[i], HIGH);
            digitalWrite(ledPinMapping[beat_step_index], LOW);  //led blinking on current step
            _delay_us(1);                                       //led blinking on current step
            digitalWrite(ledPinMapping[beat_step_index], HIGH); //led blinking on current step
        }


    for (int i = ( scaled ); i < NUMBER_OF_STEPS; i++)
        {
            digitalWrite(ledPinMapping[i], LOW);
        }
}


void potToLED2(int _pot_value)
{
    uint8_t index = (_pot_value) >> POT_SCALE_TO_4; //0..1023 0..3

    for (int i = (3-index); i <= (4+index); i++)  // bit shift right >> 7 for scaling pot value from 0..1023 to 0..7
        {
            digitalWrite(ledPinMapping[i], HIGH);
        }

    for (int i = 0; i < NUMBER_OF_STEPS; i++)
        {
            digitalWrite(ledPinMapping[i], LOW);
        }

}

void setStepVariable(uint8_t voice, int * stepVarArray)
{
    for(uint8_t beat_step_index=0; beat_step_index < NUMBER_OF_STEPS; beat_step_index++)
        {
            //when button is pressed continue to next step
            _delay_us(1000);
            setStepVarIndividual(beat_step_index, stepVarArray, voice);
        }
}


void setStepVarIndividual(uint8_t beat_step_index, int* stepVarArray, uint8_t layer_index)
{
    unsigned long current_millis = 0;
    unsigned long last_beat_millis = 0;

    while(!update_pot_value())
        {
            //mapping pot value

            int stepVar = pot_value >> 2; //pot_value set each time we call readPot()

            //mapping potentio value to LED
            potToLED(pot_value, beat_step_index);

            current_millis = synth.millis();

            //turn off beat
            if(pot_value< 20) stepVar=0;

            //set step variable to current selected value
            *(stepVarArray + beat_step_index ) = stepVar;

            //play current beat for preview (ervery 500 ms)
            if (current_millis - last_beat_millis >= 500 )
                {
                    last_beat_millis = synth.millis(); // cant use millis from arduino - iox
                    soundTrigger(layer_index, stepLayer[layer_index][beat_step_index], offsetLayer[layer_index]);
                }

        }
}


static inline void animateLed()
{
    for (uint8_t i = 0; i < NUMBER_OF_STEPS; i++)
        {
            pinMode(ledPinMapping[i], OUTPUT);
        };
    for (uint8_t i = 0; i <= NUMBER_OF_STEPS; i++)
        {
            digitalWrite(ledPinMapping[i], HIGH);
            _delay_ms(30);
        };
    for (uint8_t i = 0; i <= NUMBER_OF_STEPS; i++)
        {
            digitalWrite(ledPinMapping[i], LOW);
            _delay_ms(30);
        };
}


uint8_t selectNumber()
{
    _delay_us(1000);
    while(!update_pot_value())
    {
        potToLED(pot_value, pot_value >> POT_SCALE_TO_8);
    }
    return pot_value >> POT_SCALE_TO_8;
}


void selectWave()
{
    unsigned long current_millis = 0;
    unsigned long last_beat_millis = 0;

    uint8_t layer_index = selectNumber();
    uint8_t beat_step_index = 0;

    _delay_us(1000);

    while(!update_pot_value())
    {
        potToLED(pot_value, beat_step_index);

        uint8_t wave_index = pot_value >> POT_SCALE_TO_8;

        //play current beat for preview (ervery 500 ms)
        current_millis = synth.millis();

        if (current_millis - last_beat_millis >= beat_tempo )
            {
                if (wave_index > 5) wave_index = 5;
                synth.setWave(layer_index, wave_index);
                soundTrigger(layer_index, stepLayer[layer_index][beat_step_index], offsetLayer[layer_index]);
                last_beat_millis = synth.millis(); // cant use millis from arduino - iox
                beat_step_index++;
                if (beat_step_index > (NUMBER_OF_STEPS -1)) beat_step_index = 0;
            }

    }
}

static inline void editStep()
{
    uint8_t layer_index = 0;
    uint8_t beat_step_index = 0;
    uint8_t beat_step_index_play = 0;

    uint8_t next_mode = MENU_SEL_LAYER;
    uint8_t selected_mode = MENU_SEL_LAYER;

    unsigned long current_millis = 0;
    unsigned long last_beat_millis = 0;

    while(next_mode != MENU_EXIT_SEL)
    {
        _delay_us(1000);
        //1.select layer
        while(!update_pot_value() && next_mode == MENU_SEL_LAYER )
        {
            //cancel
            if (raw_pot_value < MENU_EXIT_THRESHOLD)
            {
                potToLED(pot_value, beat_step_index_play);
                layer_index = pot_value >> POT_SCALE_TO_8;
                selected_mode = MENU_SEL_STEP;

                //play current beat for preview (ervery 500 ms)
                current_millis = synth.millis();
                if (current_millis - last_beat_millis >= beat_tempo )
                    {
                        soundTrigger(layer_index, stepLayer[layer_index][beat_step_index_play], offsetLayer[layer_index]);
                        last_beat_millis = synth.millis(); // cant use millis from arduino - iox
                        beat_step_index_play++;
                        if (beat_step_index_play > (NUMBER_OF_STEPS -1)) beat_step_index_play = 0;
                    }

            }else{
                potToLED(0, 7);
                selected_mode = MENU_EXIT_SEL;
            }
        }

        next_mode = selected_mode;

        _delay_us(1000);
        //2.select beat step index
        while(!update_pot_value() && next_mode == MENU_SEL_STEP)
        {
            //cancel
            if (raw_pot_value < MENU_EXIT_THRESHOLD)
            {
                potToLED(pot_value, pot_value >> POT_SCALE_TO_8);
                beat_step_index = pot_value >> POT_SCALE_TO_8;
                selected_mode = MENU_SEL_NOTES;

                //play current beat for preview (ervery 500 ms)
                current_millis = synth.millis();
                if (current_millis - last_beat_millis >= beat_tempo )
                    {
                        soundTrigger(layer_index, stepLayer[layer_index][beat_step_index], offsetLayer[layer_index]);
                        last_beat_millis = synth.millis(); // cant use millis from arduino - iox
                    }

            }else{
                potToLED(0, 7);
                selected_mode = MENU_SEL_LAYER;
            }


        }

        next_mode = selected_mode;


        _delay_us(1000);
        //3.set note
        if (next_mode == MENU_SEL_NOTES)
        {
            setStepVarIndividual(beat_step_index, stepLayer[layer_index], layer_index);
            selected_mode = MENU_SEL_STEP;
        }


        next_mode = selected_mode;


    }
}

static inline void selectMode()
{
    uint8_t selected_value = 0;
    _delay_us(1000);

    while(selected_value != MODE_EXIT_MENU)
    {
        _delay_us(1000);

        while (!update_pot_value())
        {
            potToLED2(pot_value);
        }

        selected_value = (pot_value >> POT_SCALE_TO_4) + 1;

        if (selected_value == MODE_SELECT_LAYER)
        {
            uint8_t idx = selectNumber();
            setStepVariable(idx, stepLayer[idx]);
        }
        else if(selected_value == MODE_CHANGE_TIMER)
        {
            uint8_t idx = selectNumber();
            synth.bendTimer(raw_pot_value);
        }else if (selected_value == MODE_CHANGE_WAVE)
        {
            selectWave();

        }
    }
}

#if INTRO_SONG == 1
static inline void playIntroSong()
{
    for (uint8_t i = 0; i < 28; i++)
        {
            synth.mTrigger(0,melody[i]+50);
            synth.mTrigger(1,melody[i]+30);
            _delay_us(125);
        }
}
#endif

void setup()
{
    pinMode(POT_PIN, INPUT);
    digitalWrite(POT_PIN, HIGH);
    pinMode(GATE_PIN, OUTPUT);
    digitalWrite(GATE_PIN, LOW);
    analogWrite(CV_PIN, 255);

    animateLed();
    setupSound();

#if INTRO_SONG == 1
    playIntroSong();
#endif


#if ENABLE_SETSTEP_ONSTART == 1

#if ENABLE_LAYER_1 == 1
    setStepVariable(0, stepLayer[0]);
#endif
#if ENABLE_LAYER_2 == 1
    setStepVariable(1, stepLayer[1]);
#endif
#if ENABLE_LAYER_3 == 1
    setStepVariable(2, stepLayer[2]);
#endif

#endif

#if ENABLE_SELECTMODE_ONSTART == 1
    selectMode();
    _delay_us(1000);
#endif

}


void loop()
{
    uint8_t pressed = update_pot_value(); //updates global pot_value variable, return true when button pressed

    if (pressed != prev_pressed && pressed == 1)
    {
        selectMode();
    }

    prev_pressed = pressed;
    beat_tempo = pot_value;

    if (synth.millis()-lastTime >= beat_tempo )
        {

          stepToLED(beat_step_num);

#if ENABLE_LAYER_1 == 1
          soundTrigger(0, stepLayer[0][beat_step_num], offsetLayer[0]);
#endif
#if ENABLE_LAYER_2 == 1
          soundTrigger(1, stepLayer[1][beat_step_num], offsetLayer[1]);
#endif
#if ENABLE_LAYER_3 == 1
          soundTrigger(2, stepLayer[2][beat_step_num], offsetLayer[2]);
#endif

          beat_step_num++;
          if (beat_step_num>(NUMBER_OF_STEPS-1)) beat_step_num = 0;
          lastTime = synth.millis(); // cant use millis from arduino - iox
        }

}
