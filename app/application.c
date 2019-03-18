#include <application.h>
#include <at.h>

#define SEND_DATA_INTERVAL         (15 * 60 * 1000)
#define FIRST_SEND_DATA            (10 * 1000)
#define MEASURE_INTERVAL           (1 * 60 * 1000)
#define APPLICATION_TASK_ID        0

BC_DATA_STREAM_FLOAT_BUFFER(sm_voltage_buffer, 8)
BC_DATA_STREAM_FLOAT_BUFFER(sm_temperature_buffer, (SEND_DATA_INTERVAL / MEASURE_INTERVAL))
BC_DATA_STREAM_INT_BUFFER(sm_orientation_buffer, 3)

bc_data_stream_t sm_voltage;
bc_data_stream_t sm_temperature;
bc_data_stream_t sm_orientation;

// LED instance
bc_led_t led;
// Button instance
bc_button_t button;
// Lora modem instance
bc_cmwx1zzabz_t lora;
// Thermometer instance
bc_tmp112_t tmp112;
// Accelerometer instance
bc_lis2dh12_t lis2dh12;
bc_dice_t dice;

bc_scheduler_task_id_t battery_measure_task_id;

enum {
    HEADER_BOOT         = 0x00,
    HEADER_UPDATE       = 0x01,
    HEADER_BUTTON_CLICK = 0x02,
    HEADER_BUTTON_HOLD  = 0x03,

} header = HEADER_BOOT;

int channel_a_overflow_count = 0;
int channel_b_overflow_count = 0;

void pulse_counter_event_handler(bc_module_sensor_channel_t channel, bc_pulse_counter_event_t event, void *event_param)
{
    (void) event_param;

    if (event == BC_PULSE_COUNTER_EVENT_OVERFLOW)
    {
        if (channel == BC_MODULE_SENSOR_CHANNEL_A)
        {
            channel_a_overflow_count++;
        }
        else
        {
            channel_b_overflow_count++;
        }
    }
}

void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    if (event == BC_BUTTON_EVENT_CLICK)
    {
        header = HEADER_BUTTON_CLICK;

        bc_scheduler_plan_now(APPLICATION_TASK_ID);
    }
    else if (event == BC_BUTTON_EVENT_HOLD)
    {
        header = HEADER_BUTTON_HOLD;

        bc_scheduler_plan_now(APPLICATION_TASK_ID);
    }
}

void tmp112_event_handler(bc_tmp112_t *self, bc_tmp112_event_t event, void *event_param)
{
    if (event == BC_TMP112_EVENT_UPDATE)
    {
        float value;

        bc_tmp112_get_temperature_celsius(self, &value);

        bc_data_stream_feed(&sm_temperature, &value);
    }
}

void battery_event_handler(bc_module_battery_event_t event, void *event_param)
{
    if (event == BC_MODULE_BATTERY_EVENT_UPDATE)
    {
        float voltage = NAN;

        bc_module_battery_get_voltage(&voltage);

        bc_data_stream_feed(&sm_voltage, &voltage);
    }
}

void lis2dh12_event_handler(bc_lis2dh12_t *self, bc_lis2dh12_event_t event, void *event_param)
{
    if (event == BC_LIS2DH12_EVENT_UPDATE)
    {
        bc_lis2dh12_result_g_t g;

        if (bc_lis2dh12_get_result_g(self, &g))
        {
            bc_dice_feed_vectors(&dice, g.x_axis, g.y_axis, g.z_axis);

            int orientation = (int) bc_dice_get_face(&dice);

            bc_data_stream_feed(&sm_orientation, &orientation);
        }
    }
}

void lora_callback(bc_cmwx1zzabz_t *self, bc_cmwx1zzabz_event_t event, void *event_param)
{
    if (event == BC_CMWX1ZZABZ_EVENT_ERROR)
    {
        bc_led_set_mode(&led, BC_LED_MODE_BLINK_FAST);
    }
    else if (event == BC_CMWX1ZZABZ_EVENT_SEND_MESSAGE_START)
    {
        bc_led_set_mode(&led, BC_LED_MODE_ON);

        bc_scheduler_plan_relative(battery_measure_task_id, 20);
    }
    else if (event == BC_CMWX1ZZABZ_EVENT_SEND_MESSAGE_DONE)
    {
        bc_led_set_mode(&led, BC_LED_MODE_OFF);
    }
    else if (event == BC_CMWX1ZZABZ_EVENT_READY)
    {
        bc_led_set_mode(&led, BC_LED_MODE_OFF);
    }
}

void battery_measure_task(void *param)
{
    if (!bc_module_battery_measure())
    {
        bc_scheduler_plan_current_now();
    }
}

bool at_send(void)
{
    bc_scheduler_plan_now(APPLICATION_TASK_ID);

    return true;
}

bool at_status(void)
{
    float value_avg = NAN;

    static const struct {
        bc_data_stream_t *stream;
        const char *name;
        int precision;
    } values[] = {
            {&sm_voltage, "Voltage", 1},
            {&sm_temperature, "Temperature", 1}
    };

    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
    {
        value_avg = NAN;

        if (bc_data_stream_get_average(values[i].stream, &value_avg))
        {
            bc_atci_printf("$STATUS: \"%s\",%.*f", values[i].name, values[i].precision, value_avg);
        }
        else
        {
            bc_atci_printf("$STATUS: \"%s\",", values[i].name);
        }
    }

    int orientation;

    if (bc_data_stream_get_median(&sm_orientation, &orientation))
    {
        bc_atci_printf("$STATUS: \"Orientation\",%d", orientation);
    }
    else
    {
        bc_atci_printf("$STATUS: \"Orientation\",", orientation);
    }

    uint32_t channel_count_a  = bc_pulse_counter_get(BC_MODULE_SENSOR_CHANNEL_A);

    bc_atci_printf("$STATUS: \"Channel A count\",%d", channel_count_a);

    uint32_t channel_count_b = bc_pulse_counter_get(BC_MODULE_SENSOR_CHANNEL_B);

    bc_atci_printf("$STATUS: \"Channel B count\",%d", channel_count_b);

    return true;
}

void application_init(void)
{
    bc_data_stream_init(&sm_voltage, 1, &sm_voltage_buffer);
    bc_data_stream_init(&sm_temperature, 1, &sm_temperature_buffer);
    bc_data_stream_init(&sm_orientation, 1, &sm_orientation_buffer);

    // Initialize LED
    bc_led_init(&led, BC_GPIO_LED, false, false);
    bc_led_set_mode(&led, BC_LED_MODE_ON);

    // Initialize Button
    bc_button_init(&button, BC_GPIO_BUTTON, BC_GPIO_PULL_DOWN, false);
    bc_button_set_event_handler(&button, button_event_handler, NULL);

    // Initialize Thermometer
    bc_tmp112_init(&tmp112, BC_I2C_I2C0, 0x49);
    bc_tmp112_set_event_handler(&tmp112, tmp112_event_handler, NULL);
    bc_tmp112_set_update_interval(&tmp112, MEASURE_INTERVAL);

    // Initialize Battery
    bc_module_battery_init();
    bc_module_battery_set_event_handler(battery_event_handler, NULL);
    battery_measure_task_id = bc_scheduler_register(battery_measure_task, NULL, 2020);

    // Initialize Accelerometer
    bc_dice_init(&dice, BC_DICE_FACE_UNKNOWN);
    bc_lis2dh12_init(&lis2dh12, BC_I2C_I2C0, 0x19);
    bc_lis2dh12_set_resolution(&lis2dh12, BC_LIS2DH12_RESOLUTION_8BIT);
    bc_lis2dh12_set_event_handler(&lis2dh12, lis2dh12_event_handler, NULL);
    bc_lis2dh12_set_update_interval(&lis2dh12, MEASURE_INTERVAL);

    // Initialize Pulse Counter
    bc_pulse_counter_init(BC_MODULE_SENSOR_CHANNEL_A, BC_PULSE_COUNTER_EDGE_FALL);
    bc_pulse_counter_set_event_handler(BC_MODULE_SENSOR_CHANNEL_A, pulse_counter_event_handler, NULL);
    bc_pulse_counter_init(BC_MODULE_SENSOR_CHANNEL_B, BC_PULSE_COUNTER_EDGE_FALL);
    bc_pulse_counter_set_event_handler(BC_MODULE_SENSOR_CHANNEL_B, pulse_counter_event_handler, NULL);

    // Initialize lora module
    bc_cmwx1zzabz_init(&lora, BC_UART_UART1);
    bc_cmwx1zzabz_set_event_handler(&lora, lora_callback, NULL);
    bc_cmwx1zzabz_set_mode(&lora, BC_CMWX1ZZABZ_CONFIG_MODE_ABP);
    bc_cmwx1zzabz_set_class(&lora, BC_CMWX1ZZABZ_CONFIG_CLASS_A);

    // Initialize AT command interface
    at_init(&led, &lora);
    static const bc_atci_command_t commands[] = {
            AT_LORA_COMMANDS,
            {"$SEND", at_send, NULL, NULL, NULL, "Immediately send packet"},
            {"$STATUS", at_status, NULL, NULL, NULL, "Show status"},
            AT_LED_COMMANDS,
            BC_ATCI_COMMAND_CLAC,
            BC_ATCI_COMMAND_HELP
    };
    bc_atci_init(commands, BC_ATCI_COMMANDS_LENGTH(commands));

    bc_scheduler_plan_absolute(APPLICATION_TASK_ID, FIRST_SEND_DATA);
}

void application_task(void *param)
{
    if (!bc_cmwx1zzabz_is_ready(&lora))
    {
        bc_scheduler_plan_current_relative(100);

        return;
    }

    static uint8_t buffer[13];

    memset(buffer, 0xff, sizeof(buffer));

    buffer[0] = header;

    float voltage_avg = NAN;

    bc_data_stream_get_average(&sm_voltage, &voltage_avg);

    if (!isnan(voltage_avg))
    {
        buffer[1] = voltage_avg * 10.f;
    }

    int orientation;

    if (bc_data_stream_get_median(&sm_orientation, &orientation))
    {
        buffer[2] = orientation;
    }

    float temperature_avg = NAN;

    bc_data_stream_get_average(&sm_temperature, &temperature_avg);

    if (!isnan(temperature_avg))
    {
        int16_t temperature_i16 = (int16_t) (temperature_avg * 10.f);

        buffer[3] = temperature_i16 >> 8;
        buffer[4] = temperature_i16;
    }

    uint32_t channel_count_a  = bc_pulse_counter_get(BC_MODULE_SENSOR_CHANNEL_A);
    uint32_t channel_count_b = bc_pulse_counter_get(BC_MODULE_SENSOR_CHANNEL_B);

    buffer[5] = channel_count_a >> 24;
    buffer[6] = channel_count_a >> 16;
    buffer[7] = channel_count_a >> 8;
    buffer[8] = channel_count_a;

    buffer[9] = channel_count_b >> 24;
    buffer[10] = channel_count_b >> 16;
    buffer[11] = channel_count_b >> 8;
    buffer[12] = channel_count_b;

    if (!bc_cmwx1zzabz_send_message(&lora, buffer, sizeof(buffer)))
    {
        bc_scheduler_plan_current_from_now(1000);

        return;
    }

    static char tmp[sizeof(buffer) * 2 + 1];

    for (size_t i = 0; i < sizeof(buffer); i++)
    {
        sprintf(tmp + i * 2, "%02x", buffer[i]);
    }

    bc_atci_printf("$SEND: %s", tmp);

    header = HEADER_UPDATE;

    bc_scheduler_plan_current_relative(SEND_DATA_INTERVAL);
}
