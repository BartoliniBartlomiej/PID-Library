/**
 * @file basic_usage.cpp
 * @brief Demonstration of the PID Controller library capabilities with CSV export
 */

#include "pid_controller/pid_controller.hpp"
#include "pid_controller/clock_interface.hpp"
#include "pid_controller/types.hpp"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <fstream> // Dodano do obsługi plików CSV

struct ThermalPlant {
    double temperature  = 20.0;
    double ambient      = 20.0;
    double heater_power = 5.0;
    double cooling_coeff= 0.3;

    void step(double u, double dt) {
        double heating = heater_power * u;
        double cooling = cooling_coeff * (temperature - ambient);
        temperature += dt * (heating - cooling);
    }
};

static void print_header(const std::string& title) {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(70, '=') << "\n";
}

static void print_row_header() {
    std::cout << std::left
              << std::setw(6)  << "t[s]"
              << std::setw(10) << "Setpoint"
              << std::setw(10) << "Temp"
              << std::setw(10) << "Error"
              << std::setw(8)  << "Output"
              << std::setw(8)  << "P"
              << std::setw(8)  << "I"
              << std::setw(8)  << "D"
              << "\n"
              << std::string(70, '-') << "\n";
}

static void print_row(double t, double sp, double temp,
                       const pid::PIDController<double, pid::ManualClock>::DebugInfo& dbg) {
    std::cout << std::fixed << std::setprecision(2) << std::left
              << std::setw(6)  << t
              << std::setw(10) << sp
              << std::setw(10) << temp
              << std::setw(10) << dbg.error
              << std::setw(8)  << dbg.output
              << std::setw(8)  << dbg.p_term
              << std::setw(8)  << dbg.i_term
              << std::setw(8)  << dbg.d_term
              << "\n";
}

void demo_basic_pid() {
    print_header("DEMO 1: Basic Parallel PID — Step Setpoint Response");

    pid::PIDController<double, pid::ManualClock>::Config cfg{
        .gains          = {.kp = 0.8, .ki = 0.15, .kd = 0.1},
        .output_limits  = {0.0, 1.0},
        .form           = pid::PIDForm::Parallel,
        .enable_anti_windup = true
    };

    pid::ManualClock clock;
    pid::PIDController<double, pid::ManualClock> pid(cfg, clock);
    ThermalPlant plant;

    const double setpoint = 35.0;
    const double dt_sec   = 0.5;
    const int    steps    = 30;

    // Przygotowanie pliku CSV
    std::ofstream csv("demo1_basic.csv");
    csv << "time,setpoint,temperature,output\n";

    print_row_header();

    for (int i = 0; i < steps; ++i) {
        clock.advance(std::chrono::duration<double>(dt_sec));
        // pid.get_clock().advance(std::chrono::duration<double>(dt_sec));
        double u = pid.compute(setpoint, plant.temperature);
        plant.step(u, dt_sec);
        
        auto dbg = pid.get_debug_info();
        double t = i * dt_sec;
        print_row(t, setpoint, plant.temperature, dbg);
        
        // Zapis do CSV
        csv << t << "," << setpoint << "," << plant.temperature << "," << dbg.output << "\n";
    }
}

void demo_anti_windup() {
    print_header("DEMO 2: Anti-Windup — Preventing Integrator Saturation");

    auto make_pid = [](bool anti_windup) {
        pid::PIDController<double, pid::ManualClock>::Config cfg{
            .gains          = {.kp = 1.0, .ki = 0.5, .kd = 0.0},
            .output_limits  = {0.0, 1.0},
            .form           = pid::PIDForm::Parallel,
            .enable_anti_windup = anti_windup
        };
        pid::ManualClock clk;
        return std::make_pair(pid::PIDController<double, pid::ManualClock>(cfg, clk), clk);
    };

    auto [pid_aw, clk_aw]   = make_pid(true);
    auto [pid_now, clk_now] = make_pid(false);

    ThermalPlant plant_aw, plant_now;
    const double sp = 50.0;
    const double dt = 0.5;

    std::ofstream csv("demo2_anti_windup.csv");
    csv << "time,setpoint,temp_aw,temp_now\n";

    std::cout << std::left << std::setw(6) << "t[s]"
              << std::setw(20) << "Temp (anti-windup)"
              << std::setw(20) << "Temp (no anti-windup)" << "\n"
              << std::string(50, '-') << "\n";

    for (int i = 0; i < 25; ++i) {
        clk_aw.advance(std::chrono::duration<double>(dt));
        clk_now.advance(std::chrono::duration<double>(dt));

        double u_aw  = pid_aw.compute(sp, plant_aw.temperature);
        double u_now = pid_now.compute(sp, plant_now.temperature);

        plant_aw.step(u_aw, dt);
        plant_now.step(u_now, dt);

        double t = i * dt;
        std::cout << std::fixed << std::setprecision(2) << std::left
                  << std::setw(6)  << t
                  << std::setw(20) << plant_aw.temperature
                  << std::setw(20) << plant_now.temperature << "\n";

        csv << t << "," << sp << "," << plant_aw.temperature << "," << plant_now.temperature << "\n";
    }
}

void demo_gain_tuning() {
    print_header("DEMO 3: Runtime Gain Tuning");

    pid::PIDController<double, pid::ManualClock>::Config cfg{
        .gains         = {.kp = 0.3, .ki = 0.05, .kd = 0.0},
        .output_limits = {0.0, 1.0},
        .form          = pid::PIDForm::Parallel
    };
    pid::ManualClock clock;
    pid::PIDController<double, pid::ManualClock> pid(cfg, clock);

    ThermalPlant plant;
    const double sp = 30.0;
    const double dt = 0.5;

    std::ofstream csv("demo3_gain_tuning.csv");
    csv << "time,setpoint,temperature\n";

    std::cout << "Steps 0-14: conservative gains\nSteps 15+:  aggressive gains\n\n";

    for (int i = 0; i < 25; ++i) {
        if (i == 15) {
            pid.set_gains({.kp = 1.2, .ki = 0.30, .kd = 0.05});
            std::cout << "  <<< Gains updated >>>\n";
        }

        clock.advance(std::chrono::duration<double>(dt));
        double u = pid.compute(sp, plant.temperature);
        plant.step(u, dt);

        double t = i * dt;
        std::cout << std::fixed << std::setprecision(2) << std::left
                  << std::setw(6)  << t << std::setw(12) << plant.temperature << "\n";

        csv << t << "," << sp << "," << plant.temperature << "\n";
    }
}

void demo_series_vs_parallel() {
    print_header("DEMO 4: Parallel vs Series (Interactive) Form");

    pid::PIDController<double, pid::ManualClock>::Config cfg{
        .gains         = {.kp = 0.8, .ki = 0.15, .kd = 0.1},
        .output_limits = {0.0, 100.0}, // Zwiększyłem limity, żeby było coś widać
        .form          = pid::PIDForm::Parallel
    };

    // Zegary i kontrolery tworzymy bezpośrednio w funkcji demo
    pid::ManualClock clk_par;
    pid::PIDController<double, pid::ManualClock> pid_par(cfg, clk_par);

    cfg.form = pid::PIDForm::Series;
    pid::ManualClock clk_ser;
    pid::PIDController<double, pid::ManualClock> pid_ser(cfg, clk_ser);

    ThermalPlant p_par, p_ser;
    const double sp = 35.0;
    const double dt = 0.5;

    std::ofstream csv("demo4_series_vs_parallel.csv");
    csv << "time,setpoint,temp_par,temp_ser\n";

    std::cout << "Simulating... (check demo4_series_vs_parallel.csv for results)\n";

    for (int i = 0; i < 40; ++i) { // Więcej kroków
        clk_par.advance(std::chrono::duration<double>(dt));
        clk_ser.advance(std::chrono::duration<double>(dt));

        p_par.step(pid_par.compute(sp, p_par.temperature), dt);
        p_ser.step(pid_ser.compute(sp, p_ser.temperature), dt);

        double t = i * dt;
        csv << t << "," << sp << "," << p_par.temperature << "," << p_ser.temperature << "\n";
        
        // Opcjonalnie: dodaj print, żeby widzieć, że coś się dzieje
        if (i % 10 == 0) std::cout << "T=" << t << "s | Par: " << p_par.temperature << " | Ser: " << p_ser.temperature << "\n";
    }
}

void demo_derivative_filter() {
    print_header("DEMO 5: Derivative Filter — Reducing Noise Sensitivity");

    pid::PIDController<double, pid::ManualClock>::Config cfg{
        .gains                   = {.kp = 0.8, .ki = 0.1, .kd = 0.3},
        .output_limits           = {-50.0, 50.0},
        .derivative_filter_coeff = 0.0 // Dla pierwszego kontrolera
    };

    pid::ManualClock clk_nf;
    pid::PIDController<double, pid::ManualClock> pid_nf(cfg, clk_nf);

    cfg.derivative_filter_coeff = 0.8; // Dla drugiego kontrolera
    pid::ManualClock clk_f;
    pid::PIDController<double, pid::ManualClock> pid_f(cfg, clk_f);

    ThermalPlant plant_nf, plant_f;
    const double sp = 30.0;
    const double dt = 0.5;
    unsigned seed = 42;

    auto noise = [&]() -> double {
        seed = seed * 1664525u + 1013904223u;
        return ((seed >> 16) & 0xFFFF) / 65535.0 * 0.5 - 0.25;
    };

    std::ofstream csv("demo5_derivative_filter.csv");
    csv << "time,d_term_raw,d_term_filtered\n";

    std::cout << "Simulating noise... (check demo5_derivative_filter.csv)\n";

    for (int i = 0; i < 30; ++i) {
        clk_nf.advance(std::chrono::duration<double>(dt));
        clk_f.advance(std::chrono::duration<double>(dt));

        double n = noise();
        pid_nf.compute(sp, plant_nf.temperature + n);
        pid_f.compute(sp, plant_f.temperature + n);

        plant_nf.step(pid_nf.get_debug_info().output, dt);
        plant_f.step(pid_f.get_debug_info().output, dt);

        double t = i * dt;
        csv << t << "," << pid_nf.get_debug_info().d_term << "," << pid_f.get_debug_info().d_term << "\n";
    }
}

int main() {
    demo_basic_pid();
    demo_anti_windup();
    demo_gain_tuning();
    demo_series_vs_parallel();
    demo_derivative_filter();
    std::cout << "\nWszystkie dema zakończone. Wygenerowano pliki CSV do wykresów.\n";
    return 0;
}