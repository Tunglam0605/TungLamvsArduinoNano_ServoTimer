#include "competition.h"

#include "board.h"
#include "config.h"
#include "input.h"
#include "target.h"

#include <avr/io.h>

namespace {
enum class SystemState : uint8_t { Idle = 0, Running, WaitAllOff };
enum class Part1State : uint8_t { Show = 0, Hide };
enum class Part2State : uint8_t { WaitSideButton = 0, CenterVisible };
enum class Part3Target1State : uint8_t { Visible = 0, Hidden };

CompetitionMode g_mode = CompetitionMode::Part1;
SystemState g_systemState = SystemState::Idle;
uint8_t g_previousSwitchMask = COMPETITION_SWITCH_NONE;
bool g_selectionReady = false;

struct Part1Context {
    uint8_t round;
    Part1State state;
    uint32_t stateStartMs;
} g_part1{};

struct Part2Context {
    Part2State state;
    uint32_t stateStartMs;
    uint16_t lfsr;
} g_part2{Part2State::WaitSideButton, 0, 0xACE1U};

struct Part3Context {
    uint8_t target1Cycle;
    Part3Target1State target1State;
    uint32_t target1StateStartMs;
    uint32_t target2StartMs;
    bool target2Active;
} g_part3{};

constexpr TargetId kPart1Sequence[Config::kPart1RoundCount][2] = {
    {TargetId::P1_T1, TargetId::P1_T3},
    {TargetId::P1_T2, TargetId::P1_T4},
    {TargetId::P1_T2, TargetId::P1_T3},
    {TargetId::P1_T1, TargetId::P1_T4},
    {TargetId::P1_T1, TargetId::P1_T2},
    {TargetId::P1_T3, TargetId::P1_T4}
};

bool Elapsed(uint32_t nowMs, uint32_t startMs, uint32_t durationMs) {
    return static_cast<uint32_t>(nowMs - startMs) >= durationMs;
}

bool HasExactlyOneSelection(uint8_t selectionMask) {
    return selectionMask != COMPETITION_SWITCH_NONE &&
        (selectionMask & static_cast<uint8_t>(selectionMask - 1U)) == 0U;
}

CompetitionMode ModeFromSelection(uint8_t selectionMask) {
    if (selectionMask == COMPETITION_SWITCH_PART2) {
        return CompetitionMode::Part2;
    }
    if (selectionMask == COMPETITION_SWITCH_PART3) {
        return CompetitionMode::Part3;
    }
    return CompetitionMode::Part1;
}

uint8_t SelectionForMode(CompetitionMode mode) {
    return static_cast<uint8_t>(1U << static_cast<uint8_t>(mode));
}

void SetLockedModeLed(CompetitionMode mode) {
    for (uint8_t i = 0; i < 3U; ++i) {
        if (i == static_cast<uint8_t>(mode)) {
            Gpio_High(Board::kModeLedPins[i]);
        } else {
            Gpio_Low(Board::kModeLedPins[i]);
        }
    }
}

void ClearModeLeds() {
    for (const auto& led : Board::kModeLedPins) {
        Gpio_Low(led);
    }
}

void FinishCompetition() {
    Target_AllDown();
    g_systemState = SystemState::WaitAllOff;
}

void StartPart1(uint32_t nowMs) {
    Target_AllDown();
    g_part1.round = 0;
    g_part1.state = Part1State::Show;
    g_part1.stateStartMs = nowMs;
    Target_Set(kPart1Sequence[0][0], TargetState::Up);
    Target_Set(kPart1Sequence[0][1], TargetState::Up);
}

void UpdatePart1(uint32_t nowMs) {
    if (g_part1.state == Part1State::Show) {
        if (!Elapsed(nowMs, g_part1.stateStartMs, Config::kPart1VisibleMs)) {
            return;
        }
        Target_AllDown();
        g_part1.state = Part1State::Hide;
        g_part1.stateStartMs = nowMs;
        return;
    }

    if (!Elapsed(nowMs, g_part1.stateStartMs, Config::kPart1HiddenMs)) {
        return;
    }

    ++g_part1.round;
    if (g_part1.round >= Config::kPart1RoundCount) {
        FinishCompetition();
        return;
    }

    Target_Set(kPart1Sequence[g_part1.round][0], TargetState::Up);
    Target_Set(kPart1Sequence[g_part1.round][1], TargetState::Up);
    g_part1.state = Part1State::Show;
    g_part1.stateStartMs = nowMs;
}

uint16_t LfsrNext(uint16_t value) {
    const uint16_t bit = static_cast<uint16_t>(((value >> 0U) ^ (value >> 2U) ^
                                                (value >> 3U) ^ (value >> 5U)) & 1U);
    return static_cast<uint16_t>((value >> 1U) | (bit << 15U));
}

void StartPart2(uint32_t nowMs) {
    Target_AllDown();
    Target_Set(TargetId::P2_LEFT, TargetState::Up);
    Target_Set(TargetId::P2_RIGHT, TargetState::Up);
    Target_Set(TargetId::P2_CENTER, TargetState::Down);
    g_part2.state = Part2State::WaitSideButton;
    g_part2.stateStartMs = nowMs;
    g_part2.lfsr ^= static_cast<uint16_t>(nowMs) ^ static_cast<uint16_t>(TCNT0);
    if (g_part2.lfsr == 0U) {
        g_part2.lfsr = 0xACE1U;
    }
}

void UpdatePart2(uint32_t nowMs, uint8_t pressedEdges) {
    if (g_part2.state == Part2State::WaitSideButton) {
        const uint8_t sideButtons = static_cast<uint8_t>(
            COMPETITION_SWITCH_PART1 | COMPETITION_SWITCH_PART3);
        if ((pressedEdges & sideButtons) == 0U) {
            return;
        }

        g_part2.lfsr ^= static_cast<uint16_t>(nowMs) ^ static_cast<uint16_t>(TCNT0);
        g_part2.lfsr = LfsrNext(g_part2.lfsr);

        const bool chooseRight = (g_part2.lfsr & 1U) != 0U;
        Target_Set(chooseRight ? TargetId::P2_RIGHT : TargetId::P2_LEFT, TargetState::Down);
        Target_Set(TargetId::P2_CENTER, TargetState::Up);

        g_part2.state = Part2State::CenterVisible;
        g_part2.stateStartMs = nowMs;
        return;
    }

    if (Elapsed(nowMs, g_part2.stateStartMs, Config::kPart2CenterVisibleMs)) {
        Target_Set(TargetId::P2_CENTER, TargetState::Down);
        FinishCompetition();
    }
}

void StartPart3(uint32_t nowMs) {
    Target_AllDown();
    g_part3.target1Cycle = 0;
    g_part3.target1State = Part3Target1State::Visible;
    g_part3.target1StateStartMs = nowMs;
    g_part3.target2StartMs = nowMs;
    g_part3.target2Active = true;

    Target_Set(TargetId::P3_T1, TargetState::Up);
    Target_Set(TargetId::P3_T2, TargetState::Up);
}

void UpdatePart3(uint32_t nowMs) {
    if (g_part3.target2Active && Elapsed(nowMs, g_part3.target2StartMs, Config::kPart3Target2VisibleMs)) {
        Target_Set(TargetId::P3_T2, TargetState::Down);
        g_part3.target2Active = false;
    }

    if (g_part3.target1State == Part3Target1State::Visible) {
        if (!Elapsed(nowMs, g_part3.target1StateStartMs, Config::kPart3Target1VisibleMs)) {
            return;
        }
        Target_Set(TargetId::P3_T1, TargetState::Down);
        g_part3.target1State = Part3Target1State::Hidden;
        g_part3.target1StateStartMs = nowMs;
        return;
    }

    if (!Elapsed(nowMs, g_part3.target1StateStartMs, Config::kPart3Target1HiddenMs)) {
        return;
    }

    ++g_part3.target1Cycle;
    if (g_part3.target1Cycle >= Config::kPart3Target1RepeatCount) {
        FinishCompetition();
        return;
    }

    Target_Set(TargetId::P3_T1, TargetState::Up);
    g_part3.target1State = Part3Target1State::Visible;
    g_part3.target1StateStartMs = nowMs;
}

void StartSelected(uint32_t nowMs) {
    g_systemState = SystemState::Running;
    switch (g_mode) {
        case CompetitionMode::Part1: StartPart1(nowMs); break;
        case CompetitionMode::Part2: StartPart2(nowMs); break;
        case CompetitionMode::Part3: StartPart3(nowMs); break;
        default: FinishCompetition(); break;
    }
}
}

void Competition_Init() {
    for (const auto& led : Board::kModeLedPins) {
        Gpio_Output(led);
        Gpio_Low(led);
    }
    g_previousSwitchMask = Input_GetCompetitionSwitchMask();
    g_mode = CompetitionMode::Part1;
    g_systemState = SystemState::Idle;
    g_selectionReady = false;
    ClearModeLeds();
    Target_AllDown();
}

void Competition_Update(uint32_t nowMs) {
    const uint8_t switchMask = Input_GetCompetitionSwitchMask();
    const uint8_t pressedEdges = static_cast<uint8_t>(switchMask &
        static_cast<uint8_t>(~g_previousSwitchMask));
    g_previousSwitchMask = switchMask;

    if (g_systemState == SystemState::WaitAllOff) {
        if (switchMask == COMPETITION_SWITCH_NONE) {
            g_systemState = SystemState::Idle;
            g_selectionReady = true;
            ClearModeLeds();
        }
        return;
    }

    if (g_systemState == SystemState::Idle) {
        if (switchMask == COMPETITION_SWITCH_NONE) {
            g_selectionReady = true;
            return;
        }

        if (!g_selectionReady || !HasExactlyOneSelection(switchMask)) {
            g_selectionReady = false;
            return;
        }

        g_mode = ModeFromSelection(switchMask);
        g_selectionReady = false;
        SetLockedModeLed(g_mode);
        StartSelected(nowMs);
        return;
    }

    if ((switchMask & SelectionForMode(g_mode)) == 0U) {
        FinishCompetition();
        ClearModeLeds();
        return;
    }

    switch (g_mode) {
        case CompetitionMode::Part1: UpdatePart1(nowMs); break;
        case CompetitionMode::Part2: UpdatePart2(nowMs, pressedEdges); break;
        case CompetitionMode::Part3: UpdatePart3(nowMs); break;
        default: FinishCompetition(); break;
    }
}

CompetitionMode Competition_GetMode() {
    return g_mode;
}

bool Competition_IsRunning() {
    return g_systemState == SystemState::Running;
}

bool Competition_IsWaitingForAllOff() {
    return g_systemState == SystemState::WaitAllOff;
}
