
#include "input.h"
#include "Utils/Logger.h"
#include "console.h"
#include "game.h"
#include "nx.h"
#include "settings.h"
#include "sound/SoundManager.h"
#include <SDL.h>
#include <array>
#include <iostream> // Include für Debugging-Ausgaben
#include <map>
#include <string>

extern std::map<std::string, std::string> keyboardBindings;
extern std::map<std::string, std::string> gamepadBindings;

in_action mappings[INPUT_COUNT];
bool inputs[INPUT_COUNT];
bool lastinputs[INPUT_COUNT];
in_action last_sdl_action;
SDL_Joystick *joy;

int ACCEPT_BUTTON = JUMPKEY;
int DECLINE_BUTTON = FIREKEY;

bool input_init(void)
{
    memset(inputs, 0, sizeof(inputs));
    memset(lastinputs, 0, sizeof(lastinputs));
    memset(mappings, -1, sizeof(mappings));
    for (int i = 0; i < INPUT_COUNT; i++)
    {
        mappings[i].key = -1;
        mappings[i].jbut = -1;
        mappings[i].jhat = -1;
        mappings[i].jaxis = -1;
    }

    // default mappings
    mappings[LEFTKEY].key = SDLK_LEFT;
    mappings[RIGHTKEY].key = SDLK_RIGHT;
    mappings[UPKEY].key = SDLK_UP;
    mappings[DOWNKEY].key = SDLK_DOWN;
    mappings[JUMPKEY].key = SDLK_z;
    mappings[FIREKEY].key = SDLK_x;
    mappings[STRAFEKEY].key = SDLK_c;
    mappings[PREVWPNKEY].key = SDLK_a;
    mappings[NEXTWPNKEY].key = SDLK_s;
    mappings[INVENTORYKEY].key = SDLK_q;
    mappings[MAPSYSTEMKEY].key = SDLK_w;

    mappings[ESCKEY].key = SDLK_ESCAPE;

#if defined(__VITA__)
    mappings[MAPSYSTEMKEY].jbut = 0; // Triangle
    mappings[FIREKEY].jbut = 1;      // Circle
    mappings[JUMPKEY].jbut = 2;      // Cross
    mappings[INVENTORYKEY].jbut = 3; // Square
    mappings[PREVWPNKEY].jbut = 4;   // LTrig
    mappings[NEXTWPNKEY].jbut = 5;   // Rtrig

    mappings[DOWNKEY].jbut = 6;
    mappings[LEFTKEY].jbut = 7;
    mappings[UPKEY].jbut = 8;
    mappings[RIGHTKEY].jbut = 9;
    mappings[ESCKEY].jbut = 11;
#endif

#if defined(__SWITCH__)
    mappings[LEFTKEY].jbut = 16;
    mappings[UPKEY].jbut = 17;
    mappings[RIGHTKEY].jbut = 18;
    mappings[DOWNKEY].jbut = 19;

    mappings[FIREKEY].jbut = 1;  // A
    mappings[JUMPKEY].jbut = 0;  // B
    mappings[MAPSYSTEMKEY].jbut = 3;  // X
    mappings[INVENTORYKEY].jbut = 2;  // Y

    mappings[PREVWPNKEY].jbut = 6;  // L
    mappings[NEXTWPNKEY].jbut = 7;  // R

    mappings[ENTERKEY].jbut = 10; // +
    mappings[ESCKEY].jbut = 11;   // -
#endif

    mappings[F1KEY].key = SDLK_F1;
    mappings[F2KEY].key = SDLK_F2;
    mappings[F3KEY].key = SDLK_F3;
    mappings[F4KEY].key = SDLK_F4;
    mappings[F5KEY].key = SDLK_F5;
    mappings[F6KEY].key = SDLK_F6;
    mappings[F7KEY].key = SDLK_F7;
    mappings[F8KEY].key = SDLK_F8;
    mappings[F9KEY].key = SDLK_F9;
    mappings[F10KEY].key = SDLK_F10;
    mappings[F11KEY].key = SDLK_F11;
    mappings[F12KEY].key = SDLK_F12;
    mappings[FREEZE_FRAME_KEY].key = SDLK_SPACE;
    mappings[FRAME_ADVANCE_KEY].key = SDLK_b;
    mappings[DEBUG_FLY_KEY].key = SDLK_v;
    mappings[ENTERKEY].key = SDLK_RETURN;

    SDL_InitSubSystem(SDL_INIT_JOYSTICK);
    if (SDL_NumJoysticks() > 0)
    {
        // Open joystick
        joy = SDL_JoystickOpen(0);

        if (joy)
        {
            LOG_INFO("Opened Joystick 0");
            LOG_INFO("Name: {}", SDL_JoystickNameForIndex(0));
            LOG_INFO("Number of Axes: {}", SDL_JoystickNumAxes(joy));
            LOG_INFO("Number of Buttons: {}", SDL_JoystickNumButtons(joy));
            LOG_INFO("Number of Balls: {}", SDL_JoystickNumBalls(joy));
        }
        else
        {
            LOG_WARN("Couldn't open Joystick 0");
        }
    }
    return 0;
}

void rumble(float str, uint32_t len)
{
    if (settings->rumble)
        SDL_JoystickRumble(joy, 0xFFFF * str, 0xFFFF * str, len);
}

// set the SDL key that triggers an input
void input_remap(int keyindex, in_action sdl_key)
{
    LOG_DEBUG("input_remap(%d => %d)", keyindex, sdl_key.key);
    mappings[keyindex] = sdl_key;
}

// get which SDL key triggers a given input
in_action input_get_mapping(int keyindex)
{
    return mappings[keyindex];
}

// Deklaration der input_get_action Funktion mit Kommentaren und Debugging-Informationen
int input_get_action(int32_t sdlkey)
{
    // Debugging: Ausgabe des empfangenen Tastencodes
    std::cout << "Empfangener Tastencode: " << sdlkey << std::endl;
    // Durchlaufen des mappings-Arrays, das Tastencodes zu Aktionen zuordnet
    for (int i = 0; i < INPUT_COUNT; i++)
    {
        // Überprüfung, ob der aktuelle Tastencode im mappings Array vorhanden ist
        if (mappings[i].key == sdlkey)
        {
            // Debugging: Ausgabe der gefundenen Aktion
            std::cout << "Zugeordnete Aktion: " << i << " für Tastencode: " << sdlkey << std::endl;
            return i; // Rückgabe des Indexes der Aktion, wenn eine Übereinstimmung gefunden wurde
        }
    }
    // Debugging: Ausgabe, wenn keine Aktion für den Tastencode gefunden wird
    std::cout << "Keine Aktion für Tastencode gefunden: " << sdlkey << std::endl;
    return -1; // Rückgabe von -1, wenn keine Aktion zugeordnet ist
}

int input_get_action_but(int32_t jbut)
{
    for (int i = 0; i < INPUT_COUNT; i++)
    {
        if (mappings[i].jbut == jbut)
        {
            return i;
        }
    }
    return -1;
}

int input_get_action_hat(int32_t jhat, int32_t jvalue)
{
    for (int i = 0; i < INPUT_COUNT; i++)
    {
        if ((mappings[i].jhat == jhat) && (jvalue & mappings[i].jhat_value))
        {
            return i;
        }
    }
    return -1;
}

int input_get_action_axis(int32_t jaxis, int32_t jvalue)
{
    for (int i = 0; i < INPUT_COUNT; i++)
    {
#include "input.h"
#include "Utils/Logger.h"
#include "console.h"
#include "game.h"
#include "nx.h"
#include "settings.h"
#include "sound/SoundManager.h"
#include <SDL.h>
#include <array>
#include <iostream> // Include für Debugging-Ausgaben

in_action mappings[INPUT_COUNT];
bool inputs[INPUT_COUNT];
bool lastinputs[INPUT_COUNT];
in_action last_sdl_action;
SDL_Joystick *joy;

int ACCEPT_BUTTON = JUMPKEY;
int DECLINE_BUTTON = FIREKEY;

bool input_init(void)
{
    memset(inputs, 0, sizeof(inputs));
    memset(lastinputs, 0, sizeof(lastinputs));
    memset(mappings, -1, sizeof(mappings));
    for (int i = 0; i < INPUT_COUNT; i++)
    {
        mappings[i].key = -1;
        mappings[i].jbut = -1;
        mappings[i].jhat = -1;
        mappings[i].jaxis = -1;
    }

    // default mappings
    mappings[LEFTKEY].key = SDLK_LEFT;
    mappings[RIGHTKEY].key = SDLK_RIGHT;
    mappings[UPKEY].key = SDLK_UP;
    mappings[DOWNKEY].key = SDLK_DOWN;
    mappings[JUMPKEY].key = SDLK_z;
    mappings[FIREKEY].key = SDLK_x;
    mappings[STRAFEKEY].key = SDLK_c;
    mappings[PREVWPNKEY].key = SDLK_a;
    mappings[NEXTWPNKEY].key = SDLK_s;
    mappings[INVENTORYKEY].key = SDLK_q;
    mappings[MAPSYSTEMKEY].key = SDLK_w;

    mappings[ESCKEY].key = SDLK_ESCAPE;

#if defined(__VITA__)
    mappings[MAPSYSTEMKEY].jbut = 0; // Triangle
    mappings[FIREKEY].jbut = 1;      // Circle
    mappings[JUMPKEY].jbut = 2;      // Cross
    mappings[INVENTORYKEY].jbut = 3; // Square
    mappings[PREVWPNKEY].jbut = 4;   // LTrig
    mappings[NEXTWPNKEY].jbut = 5;   // Rtrig

    mappings[DOWNKEY].jbut = 6;
    mappings[LEFTKEY].jbut = 7;
    mappings[UPKEY].jbut = 8;
    mappings[RIGHTKEY].jbut = 9;
    mappings[ESCKEY].jbut = 11;
#endif

#if defined(__SWITCH__)
    mappings[LEFTKEY].jbut = 16;
    mappings[UPKEY].jbut = 17;
    mappings[RIGHTKEY].jbut = 18;
    mappings[DOWNKEY].jbut = 19;

    mappings[FIREKEY].jbut = 1;  // A
    mappings[JUMPKEY].jbut = 0;  // B
    mappings[MAPSYSTEMKEY].jbut = 3;  // X
    mappings[INVENTORYKEY].jbut = 2;  // Y

    mappings[PREVWPNKEY].jbut = 6;  // L
    mappings[NEXTWPNKEY].jbut = 7;  // R

    mappings[ENTERKEY].jbut = 10; // +
    mappings[ESCKEY].jbut = 11;   // -
#endif

    mappings[F1KEY].key = SDLK_F1;
    mappings[F2KEY].key = SDLK_F2;
    mappings[F3KEY].key = SDLK_F3;
    mappings[F4KEY].key = SDLK_F4;
    mappings[F5KEY].key = SDLK_F5;
    mappings[F6KEY].key = SDLK_F6;
    mappings[F7KEY].key = SDLK_F7;
    mappings[F8KEY].key = SDLK_F8;
    mappings[F9KEY].key = SDLK_F9;
    mappings[F10KEY].key = SDLK_F10;
    mappings[F11KEY].key = SDLK_F11;
    mappings[F12KEY].key = SDLK_F12;
    mappings[FREEZE_FRAME_KEY].key = SDLK_SPACE;
    mappings[FRAME_ADVANCE_KEY].key = SDLK_b;
    mappings[DEBUG_FLY_KEY].key = SDLK_v;
    mappings[ENTERKEY].key = SDLK_RETURN;

    SDL_InitSubSystem(SDL_INIT_JOYSTICK);
    if (SDL_NumJoysticks() > 0)
    {
        // Open joystick
        joy = SDL_JoystickOpen(0);

        if (joy)
        {
            LOG_INFO("Opened Joystick 0");
            LOG_INFO("Name: {}", SDL_JoystickNameForIndex(0));
            LOG_INFO("Number of Axes: {}", SDL_JoystickNumAxes(joy));
            LOG_INFO("Number of Buttons: {}", SDL_JoystickNumButtons(joy));
            LOG_INFO("Number of Balls: {}", SDL_JoystickNumBalls(joy));
        }
        else
        {
            LOG_WARN("Couldn't open Joystick 0");
        }
    }
    return 0;
}

void rumble(float str, uint32_t len)
{
    if (settings->rumble)
        SDL_JoystickRumble(joy, 0xFFFF * str, 0xFFFF * str, len);
}

// set the SDL key that triggers an input
void input_remap(int keyindex, in_action sdl_key)
{
    LOG_DEBUG("input_remap(%d => %d)", keyindex, sdl_key.key);
    mappings[keyindex] = sdl_key;
}

// get which SDL key triggers a given input
in_action input_get_mapping(int keyindex)
{
    return mappings[keyindex];
}

// Deklaration der input_get_action Funktion mit Kommentaren und Debugging-Informationen
int input_get_action(int32_t sdlkey)
{
    // Debugging: Ausgabe des empfangenen Tastencodes
    std::cout << "Empfangener Tastencode: " << sdlkey << std::endl;
    // Durchlaufen des mappings-Arrays, das Tastencodes zu Aktionen zuordnet
    for (int i = 0; i < INPUT_COUNT; i++)
    {
        // Überprüfung, ob der aktuelle Tastencode im mappings Array vorhanden ist
        if (mappings[i].key == sdlkey)
        {
            // Debugging: Ausgabe der gefundenen Aktion
            std::cout << "Zugeordnete Aktion: " << i << " für Tastencode: " << sdlkey << std::endl;
            return i; // Rückgabe des Indexes der Aktion, wenn eine Übereinstimmung gefunden wurde
        }
    }
    // Debugging: Ausgabe, wenn keine Aktion für den Tastencode gefunden wird
    std::cout << "Keine Aktion für Tastencode gefunden: " << sdlkey << std::endl;
    return -1; // Rückgabe von -1, wenn keine Aktion zugeordnet ist
}

int input_get_action_but(int32_t jbut)
{
    for (int i = 0; i < INPUT_COUNT; i++)
    {
        if (mappings[i].jbut == jbut)
        {
            return i;
        }
    }
    return -1;
}

int input_get_action_hat(int32_t jhat, int32_t jvalue)
{
    for (int i = 0; i < INPUT_COUNT; i++)
    {
        if ((mappings[i].jhat == jhat) && (jvalue & mappings[i].jhat_value))
        {
            return i;
        }
    }
    return -1;
}

int input_get_action_axis(int32_t jaxis, int32_t jvalue)
{
    for (int i = 0; i < INPUT_COUNT; i++)
    {
        if ((mappings[i].jaxis == jaxis)
            && ((jvalue > 0 && mappings[i].jaxis_value > 0) || (jvalue < 0 && mappings[i].jaxis_value < 0)))
        {
            return i;
        }
    }
    return -1;
}

const std::string input_get_name(int index)
{
    static std::array<std::string, 28> input_names = {"Left", "Right",
                                                      "Up", "Down",
                                                      "Jump", "Fire",
                                                      "Strafe", "Wpn Prev",
                                                      "Wpn Next", "Inventory",
                                                      "Map", "Pause",
                                                      "f1", "f2",
                                                      "f3", "f4",
                                                      "f5", "f6",
                                                      "f7", "f8",
                                                      "f9", "f10",
                                                      "f11", "f12",
                                                      "freeze frame", "frame advance",
                                                      "debug fly", "Enter"};

    if (index < 0 || index >= INPUT_COUNT)
        return "invalid";

    return input_names[index];
}

void input_set_mappings(in_action *array)
{
    for (int i = 0; i < INPUT_COUNT; i++) {
        if (array[i].key > 0 || array[i].jbut != -1 || array[i].jaxis != -1 || array[i].jhat != -1)
            mappings[i] = array[i];
    }
}

// keys that we don't want to send to the console
// even if the console is up.
static int IsNonConsoleKey(int key)
{
    static const int nosend[] = {SDLK_LEFT, SDLK_RIGHT, 0};

    for (int i = 0; nosend[i]; i++)
        if (key == nosend[i])
            return true;

    return false;
}

// Haupt-Ereignisschleife zur Verarbeitung von Tastatureingaben
void processInput() {
    SDL_Event evt;
    while (SDL_PollEvent(&evt)) {
        switch (evt.type) {
            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                int key = evt.key.keysym.sym; // Erfassung des Tastencodes
                int ino = input_get_action(key); // Zuordnung des Tastencodes zur Aktion
                // Debugging: Überprüfung, ob eine gültige Aktion gefunden wurde
                if (ino != -1) {
                    std::cout << "Aktion " << (evt.type == SDL_KEYDOWN ? "gedrückt: " : "losgelassen: ") << ino << std::endl;
                    inputs[ino] = (evt.type == SDL_KEYDOWN); // Aktualisierung des Zustands der Aktion
                } else {
                    std::cout << "Keine zugeordnete Aktion für Tastencode: " << key << std::endl;
                }
                // Sonderbehandlung für Shift-Tasten
                if (key == SDLK_LSHIFT) {
                    if (evt.type == SDL_KEYDOWN) {
                        shiftstates |= LEFTMASK;
                    } else {
                        shiftstates &= ~LEFTMASK;
                    }
                }
                break;
            }
            case SDL_QUIT: {
                inputs[ESCKEY] = true;
                game.running = false;
                break;
            }
            case SDL_JOYBUTTONDOWN: {
                Uint8 but = evt.jbutton.button;
                last_sdl_action.jbut = but;
                int ino = input_get_action_but(but); // Zuordnung des Joystick-Buttons zur Aktion
                if (ino != -1) {
                    inputs[ino] = (evt.jbutton.state == SDL_PRESSED);
                }
                break;
            }
            case SDL_JOYBUTTONUP: {
                Uint8 but = evt.jbutton.button;
                int ino = input_get_action_but(but); // Zuordnung des Joystick-Buttons zur Aktion
                if (ino != -1) {
                    inputs[ino] = (evt.jbutton.state == SDL_PRESSED);
                }
                break;
            }
            case SDL_JOYHATMOTION: {
                if (evt.jhat.value != SDL_HAT_CENTERED) {
                    last_sdl_action.jhat = evt.jhat.hat;
                    last_sdl_action.jhat_value = evt.jhat.value;
                }
                int ino = input_get_action_hat(evt.jhat.hat, evt.jhat.value); // Zuordnung des Joystick-Hats zur Aktion
                // cleanup all hat-binded states
                for (int i = 0; i < INPUT_COUNT; i++) {
                    if (mappings[i].jhat != -1)
                        inputs[i] = false;
                }

                if (ino != -1)
                    inputs[ino] = true;
                break;
            }
            case SDL_JOYAXISMOTION: {
                // this only used for control remapping.
                // for actual in-game handling see below.
                if (evt.jaxis.value > 20000 || evt.jaxis.value < -20000) { // dead zone
                    last_sdl_action.jaxis = evt.jaxis.axis;
                    last_sdl_action.jaxis_value = evt.jaxis.value;
                }
                break;
            }
        }
    }

    // handle gamepad sticks
    for (int i = 0; i < INPUT_COUNT; i++) {
        if (mappings[i].jaxis >= 0) { // reset all mapped axises
            inputs[i] = false;
        }
    }

    // now get current values of all axises
    for (int ax = 0; ax < SDL_JoystickNumAxes(joy); ax++) {
        int value = SDL_JoystickGetAxis(joy, ax);
        int ino = input_get_action_axis(ax, value); // this returns actual mapping for axis direction

        if (ino != -1 && (value > 20000 || value < -20000))
            inputs[ino] = true;
    }
}

void input_close(void)
{
    // Close if opened
    if (SDL_JoystickGetAttached(joy))
    {
        SDL_JoystickClose(joy);
    }
}

bool buttondown(void)
{
    for (int i = 0; buttons[i]; i++)
    {
        if (inputs[buttons[i]])
            return 1;
    }

    return 0;
}

bool buttonjustpushed(void)
{
    for (int i = 0; buttons[i]; i++)
    {
        if (inputs[buttons[i]] && !lastinputs[buttons[i]])
            return 1;
    }

    return 0;
}

bool justpushed(int k)
{
    return (inputs[k] && !lastinputs[k]);
}
