#include <gtest/gtest.h>

#include "MotorDataConverter.h"
#include "PwmDataConverter.h"
#include "SpeedDataConverter.h"

namespace {

using namespace spirit;

/**
 * @brief PWM制御のデータをエンコード/デコードできることのテスト
 */
TEST(MotorDataConverter, PwmDataDecodeTest)
{
    auto test = [](float duty_cycle, Motor::State state) {
        MotorDataConverter motor_data_converter;
        Motor              motor;
        motor.control_system(Motor::ControlSystem::PWM);
        motor.duty_cycle(duty_cycle);
        motor.state(state);

        constexpr std::size_t max_buffer_size = 24;
        uint8_t               buffer[max_buffer_size / 8]{};
        std::size_t           buffer_size = 0;

        motor_data_converter.encode(motor, max_buffer_size, buffer, buffer_size);

        // 2(ヘッダ) + 16(デューティー比) + 2(回転方向) = 20
        constexpr std::size_t expected_buffer_size = 20;
        EXPECT_EQ(expected_buffer_size, buffer_size);

        Motor decoded_motor;
        motor_data_converter.decode(buffer, max_buffer_size, decoded_motor);

        // デューティ比は0.0~1.0の範囲を(2^16 - 1)で割った値を送受信しているでため、誤差は1/(2^16 - 1)とする
        constexpr float allowable_error_margin = 1.0F / 65535.0F;
        EXPECT_NEAR(duty_cycle, decoded_motor.get_duty_cycle(), allowable_error_margin);
        EXPECT_EQ(state, decoded_motor.get_state());
    };

    // とりあえず、test_PwmDataConverter.cppのテストを持ってきた

    // デューティ比が0未満や1より大きい場合のテストはMotorクラスのテストで行う
    // とりあえず、デューティ比が最小/中間/適当な細かい値/最大と、各回転方向のテストを行う
    test(0.00F, Motor::State::Coast);
    test(0.00F, Motor::State::CW);
    test(0.00F, Motor::State::CCW);
    test(0.00F, Motor::State::Brake);

    test(0.50F, Motor::State::Coast);
    test(0.50F, Motor::State::CW);
    test(0.50F, Motor::State::CCW);
    test(0.50F, Motor::State::Brake);

    test(0.123456F, Motor::State::Coast);
    test(0.123456F, Motor::State::CW);
    test(0.123456F, Motor::State::CCW);
    test(0.123456F, Motor::State::Brake);

    test(1.00F, Motor::State::Coast);
    test(1.00F, Motor::State::CW);
    test(1.00F, Motor::State::CCW);
    test(1.00F, Motor::State::Brake);
}

/**
 * @brief 速度制御のデータをエンコード/デコードできることのテスト
 */
TEST(MotorDataConverter, SpeedDataTest)
{
    auto test = [](float speed, float Kp, float Ki, Motor::State state) {
        MotorDataConverter motor_data_converter;
        Motor              motor;
        motor.control_system(Motor::ControlSystem::Speed);
        motor.speed(speed);
        motor.pid_gain_factor(Kp, Ki, 0.0F);
        motor.state(state);

        constexpr std::size_t max_buffer_size = 56;
        uint8_t               buffer[max_buffer_size / 8]{};
        std::size_t           buffer_size = 0;

        motor_data_converter.encode(motor, max_buffer_size, buffer, buffer_size);

        // 2(ヘッダ) + 16(スピード(rps)) + 16(Kp) + 16(Ki) + 2(回転方向) = 52
        constexpr std::size_t expected_buffer_size = 52;
        EXPECT_EQ(expected_buffer_size, buffer_size);

        Motor decoded_motor;
        motor_data_converter.decode(buffer, max_buffer_size, decoded_motor);

        // スピード, Kp, Kiは送受信中にbfloat16に変換している
        // bfloat16の仮数部は7bitなので、その分の誤差を考慮する必要がある
        auto allowable_error_margin = [](const float bfloat16) { return bfloat16 / 127.0F; };

        EXPECT_NEAR(speed, decoded_motor.get_speed(), allowable_error_margin(speed));

        float return_Kp = 0.0F;
        float return_Ki = 0.0F;
        float return_Kd = 0.0F;
        decoded_motor.get_pid_gain_factor(return_Kp, return_Ki, return_Kd);
        EXPECT_NEAR(Kp, return_Kp, allowable_error_margin(Kp));
        EXPECT_NEAR(Ki, return_Ki, allowable_error_margin(Ki));

        EXPECT_EQ(state, decoded_motor.get_state());
    };

    // とりあえず、test_SpeedDataConverter.cppのテストを持ってきた

    // スピードが0未満や1より大きい場合のテストはMotorクラスのテストで行う
    // とりあえず、スピードが最小/中間/適当な細かい値/最大と、各回転方向のテストを行う
    test(0.00F, 0.00F, 0.00F, Motor::State::Coast);
    test(0.00F, 0.00F, 0.00F, Motor::State::CW);
    test(0.00F, 0.00F, 0.00F, Motor::State::CCW);
    test(0.00F, 0.00F, 0.00F, Motor::State::Brake);

    test(0.50F, 0.00F, 0.00F, Motor::State::Coast);
    test(0.50F, 0.00F, 0.00F, Motor::State::Coast);
    test(0.50F, 0.00F, 0.00F, Motor::State::Coast);
    test(0.50F, 0.00F, 0.00F, Motor::State::Coast);

    test(0.00F, 0.50F, 0.00F, Motor::State::Coast);
    test(0.00F, 0.50F, 0.00F, Motor::State::Coast);
    test(0.00F, 0.50F, 0.00F, Motor::State::Coast);
    test(0.00F, 0.50F, 0.00F, Motor::State::Coast);

    test(0.00F, 0.00F, 0.50F, Motor::State::Coast);
    test(0.00F, 0.00F, 0.50F, Motor::State::Coast);
    test(0.00F, 0.00F, 0.50F, Motor::State::Coast);
    test(0.00F, 0.00F, 0.50F, Motor::State::Coast);

    test(0.50F, 0.05F, 1.00F, Motor::State::CW);
    test(1.00F, 0.75F, 0.20F, Motor::State::CCW);
}

/**
 * @brief デコードするデータが現在対応しているヘッダであればtrueを返し、
 * それ以外であればfalseを返すことの確認
 */
TEST(MotorDataConverter, DecodeErrorTest)
{
    // 正常な場合
    MotorDataConverter    motor_data_converter;
    Motor                 motor;
    constexpr std::size_t buffer_size = 64;
    uint8_t               buffer[buffer_size / 8]{0x00};

    /// @test PWM制御(ヘッダ以外の添え字0のbitは0)
    buffer[0] = 0x00 | 0x00F;
    EXPECT_TRUE(motor_data_converter.decode(buffer, buffer_size, motor));
    EXPECT_EQ(motor.get_control_system(), Motor::ControlSystem::PWM);

    /// @test PWM制御(ヘッダ以外の添え字0のbitは1)
    buffer[0] = 0x00 | 0x3F;
    EXPECT_TRUE(motor_data_converter.decode(buffer, buffer_size, motor));
    EXPECT_EQ(motor.get_control_system(), Motor::ControlSystem::PWM);

    /// @test 速度制御(ヘッダ以外の添え字0のbitは0)
    buffer[0] = 0x40 | 0x0F;
    EXPECT_TRUE(motor_data_converter.decode(buffer, buffer_size, motor));
    EXPECT_EQ(motor.get_control_system(), Motor::ControlSystem::Speed);

    /// @test 速度制御(ヘッダ以外の添え字0のbitは1)
    buffer[0] = 0x40 | 0x3F;
    EXPECT_TRUE(motor_data_converter.decode(buffer, buffer_size, motor));
    EXPECT_EQ(motor.get_control_system(), Motor::ControlSystem::Speed);

    // 異常な場合
    /// @test ヘッダ部分のFAILとなる全パターンでテスト
    buffer[0] = 0x80;
    EXPECT_FALSE(motor_data_converter.decode(buffer, buffer_size, motor));
    buffer[0] = 0xC0;
    EXPECT_FALSE(motor_data_converter.decode(buffer, buffer_size, motor));
}

}  // namespace
