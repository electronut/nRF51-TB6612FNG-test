/* 
   
   nRF51-TB6612FNG-test/main.c

   Controlling motors using TB6612FNG.
   
   Demonstrates PWM and NUS (Nordic UART Service).

   Author: Mahesh Venkitachalam
   Website: electronut.in


   Reference:

   http://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk51.v9.0.0%2Findex.html

 */

#include "ble_init.h"

extern ble_nus_t m_nus;                                  

// Create the instance "PWM1" using TIMER1.
APP_PWM_INSTANCE(PWM1,1);                   

// These are based on default values sent by Nordic nRFToolbox app
// Modify as neeeded
#define FORWARD "FastForward"
#define REWIND "Rewind"
#define STOP "Stop"
#define PAUSE "Pause"
#define PLAY "Play"
#define START "Start"
#define END "End"
#define RECORD "Rec"
#define SHUFFLE "Shuffle"

// flip directions
bool flipA = false;
bool flipB = false;

// min/max motos speeds - PWM duty cycle
const uint32_t speedMin = 90;
const uint32_t speedMax = 90;
// current motor speeds
uint32_t speedA = 90;
uint32_t speedB = 90;

bool stopMotors = false;

bool changed = false;

// Function for handling the data from the Nordic UART Service.
static void nus_data_handler(ble_nus_t * p_nus, uint8_t * p_data, 
                             uint16_t length)
{
  if (strstr((char*)(p_data), RECORD)) {
    flipA = !flipA;
  }
  else if (strstr((char*)(p_data), SHUFFLE)) {
    flipB = !flipB;
  }
  else if (strstr((char*)(p_data), STOP)) {
    stopMotors = true;
  }
  else if (strstr((char*)(p_data), PLAY)) {
    stopMotors = false;
  }
  
  // set changed flag
  changed = true;
}


// A flag indicating PWM status.
static volatile bool pwmReady = false;            

// PWM callback function
void pwm_ready_callback(uint32_t pwm_id)    
{
    pwmReady = true;
}

// TB6612FNG
// motor #1 connected to A01 and A02
// motor #2 connected to B01 and B02

// Motor #1
int PWMA = 1; //Speed control 
int AIN1 = 2; //Direction
int AIN2 = 3; //Direction

// Motor #2
int PWMB = 4; //Speed control
int BIN1 = 5; //Direction
int BIN2 = 6; //Direction

int STBY = 7; //standby

void init_motors(void)
{
  // set up GPIOs
  nrf_gpio_cfg_output(STBY);
  nrf_gpio_cfg_output(AIN1);
  nrf_gpio_cfg_output(AIN2);
  nrf_gpio_cfg_output(BIN1);
  nrf_gpio_cfg_output(BIN2);
}
// Function for initializing services that will be used by the application.
void services_init()
{
    uint32_t       err_code;
    ble_nus_init_t nus_init;
    
    memset(&nus_init, 0, sizeof(nus_init));

    nus_init.data_handler = nus_data_handler;
    
    err_code = ble_nus_init(&m_nus, &nus_init);
    APP_ERROR_CHECK(err_code);
}

/* brake: sudden stop of both motors */
void brake()
{

}

/* stop_motors: bring motors to a stop */
void stop_motors()
{
  nrf_gpio_pin_clear(STBY);
}

/* set_speed: set speed for both motors */
void set_speed(uint8_t speed)
{
  // set speed
  while (app_pwm_channel_duty_set(&PWM1, 0, speed) == NRF_ERROR_BUSY);
  while (app_pwm_channel_duty_set(&PWM1, 1, speed) == NRF_ERROR_BUSY);      
}

/* direction: change motor direction */
void set_dir(bool forward)
{
  if(forward) {
    // set direction A
    nrf_gpio_pin_set(pinIN1);
    nrf_gpio_pin_clear(pinIN2);
    // set direction B
    nrf_gpio_pin_set(pinIN3);
    nrf_gpio_pin_clear(pinIN4);
  }
  else {
     // set direction A
    nrf_gpio_pin_clear(pinIN1);
    nrf_gpio_pin_set(pinIN2);
    // set direction B
    nrf_gpio_pin_clear(pinIN3);
    nrf_gpio_pin_set(pinIN4);
  }
}

#define APP_TIMER_PRESCALER  0    /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_MAX_TIMERS 6    /**< Maximum number of simultaneously created timers. */
#define APP_TIMER_OP_QUEUE_SIZE 4  /**< Size of timer operation queues. */

// Application main function.
int main(void)
{
    uint32_t err_code;

    // set up timers
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_MAX_TIMERS, 
                   APP_TIMER_OP_QUEUE_SIZE, false);
   
    // initlialize BLE
    ble_stack_init();
    gap_params_init();
    services_init();
    advertising_init();
    conn_params_init();
    // start BLE advertizing
    err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);

    // init GPIOTE
    err_code = nrf_drv_gpiote_init();
    APP_ERROR_CHECK(err_code);

    // init PPI
    err_code = nrf_drv_ppi_init();
    APP_ERROR_CHECK(err_code);

    // intialize UART
    uart_init();

    // prints to serial port
    printf("starting...\n");

    init_motors();

    // set direction
    set_dir(true);

    // 2-channel PWM
    app_pwm_config_t pwm1_cfg = 
      APP_PWM_DEFAULT_CONFIG_2CH(1000L, PWMA, PWMB);

    pwm1_cfg.pin_polarity[0] = APP_PWM_POLARITY_ACTIVE_HIGH;
    pwm1_cfg.pin_polarity[1] = APP_PWM_POLARITY_ACTIVE_HIGH;

    printf("before pwm init\n");
    /* Initialize and enable PWM. */
    err_code = app_pwm_init(&PWM1,&pwm1_cfg,pwm_ready_callback);
    APP_ERROR_CHECK(err_code);
    printf("after pwm init\n");

    app_pwm_enable(&PWM1);

    // wait till PWM is ready
    //while(!pwmReady);

    set_speed(50);

    printf("entering loop\n");
    while(1) {

      for (int i = 1; i < 10; i++) {

        set_speed(i*10);
        nrf_delay_ms(1000);
      }
      
      for (int i = 9; i > 0; i--) {

        set_speed(i*10);
        nrf_delay_ms(1000);
      }

      // delay
      nrf_delay_ms(2000);
    }
}
