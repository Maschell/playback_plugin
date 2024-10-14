#include "utils/logger.h"
#include <malloc.h>
#include <sysapp/launch.h>
#include <wups.h>

#include <notifications/notifications.h>

#include "utils/StringTools.h"
#include "utils/cafe_glyphs.h"
#include "utils/input.h"
#include <coreinit/title.h>

WUPS_PLUGIN_NAME("Record and playback plugin");
WUPS_PLUGIN_DESCRIPTION("WIP");
WUPS_PLUGIN_VERSION("v1.0");
WUPS_PLUGIN_AUTHOR("Maschell");
WUPS_PLUGIN_LICENSE("BSD");


WUPS_USE_WUT_DEVOPTAB(); // Use the wut devoptabs

#define START_RECORDING_CONFIG_ID "start_recording"
#define START_PLAYBACK_CONFIG_ID  "start_playback"


bool sIsRecording      = false;
bool sIsPlayback       = false;
bool sDoStartRecording = false;
bool sDoStartPlayback  = false;


INITIALIZE_PLUGIN() {
    // Logging only works when compiled with `make DEBUG=1`. See the README for more information.
    initLogging();
    DEBUG_FUNCTION_LINE("INITIALIZE_PLUGIN of playback plugin!");

    if (NotificationModuleStatus res; (res = NotificationModule_InitLibrary()) != NOTIFICATION_MODULE_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to init NotificationModule: %s", NotificationModule_GetStatusStr(res));
    }

    deinitLogging();
}


DEINITIALIZE_PLUGIN() {
    DEBUG_FUNCTION_LINE("DEINITIALIZE_PLUGIN of example_plugin!");
}

struct VPADStatusWrapper {
    int result;
    VPADStatus data;
    VPADReadError error;
};
std::vector<VPADStatusWrapper> controllerData;

static bool sRestartTriggered = false;

static int sPlaybackOffset                            = 0;
static NotificationModuleHandle sRecordNotification   = 0;
static NotificationModuleHandle sPlaybackNotification = 0;

ON_APPLICATION_START() {
    initLogging();
    sIsRecording = false;
    sIsPlayback  = false;

    sRestartTriggered = false;
    if (sDoStartRecording) {
        NotificationModule_AddInfoNotification("Start recording!");

        if (NotificationModule_AddDynamicNotification("Recording...", &sRecordNotification) != NOTIFICATION_MODULE_RESULT_SUCCESS) {
            DEBUG_FUNCTION_LINE_ERR("Failed to create recording notification!");
        }

        sDoStartRecording = false;
        sIsRecording      = true;
        controllerData.clear();
    }
    if (sDoStartPlayback) {
        NotificationModule_AddInfoNotification("Start playback!");

        if (NotificationModule_AddDynamicNotification("Playback...", &sPlaybackNotification) != NOTIFICATION_MODULE_RESULT_SUCCESS) {
            DEBUG_FUNCTION_LINE_ERR("Failed to create playback notification!");
        }
        sDoStartPlayback = false;
        sIsPlayback      = true;
        sPlaybackOffset  = 0;
    }

    DEBUG_FUNCTION_LINE("ON_APPLICATION_START of example_plugin!");
}


ON_APPLICATION_ENDS() {
    deinitLogging();
}


ON_APPLICATION_REQUESTS_EXIT() {
    DEBUG_FUNCTION_LINE_INFO("ON_APPLICATION_REQUESTS_EXIT of example_plugin!");
}

struct entry {
    VPADButtons button;
    const char *name;
    const char *glyph;
};


constexpr auto button_entries = {
        entry{VPAD_BUTTON_L, "VPAD_BUTTON_L", CAFE_GLYPH_GAMEPAD_BTN_L},
        entry{VPAD_BUTTON_R, "VPAD_BUTTON_R", CAFE_GLYPH_GAMEPAD_BTN_R},
        entry{VPAD_BUTTON_ZL, "VPAD_BUTTON_ZL", CAFE_GLYPH_GAMEPAD_BTN_ZL},
        entry{VPAD_BUTTON_ZR, "VPAD_BUTTON_ZR", CAFE_GLYPH_GAMEPAD_BTN_ZR},
        entry{VPAD_BUTTON_UP, "VPAD_BUTTON_UP", CAFE_GLYPH_GAMEPAD_BTN_UP},
        entry{VPAD_BUTTON_DOWN, "VPAD_BUTTON_DOWN", CAFE_GLYPH_GAMEPAD_BTN_DOWN},
        entry{VPAD_BUTTON_LEFT, "VPAD_BUTTON_LEFT", CAFE_GLYPH_GAMEPAD_BTN_LEFT},
        entry{VPAD_BUTTON_RIGHT, "VPAD_BUTTON_RIGHT", CAFE_GLYPH_GAMEPAD_BTN_RIGHT},
        entry{VPAD_BUTTON_A, "VPAD_BUTTON_A", CAFE_GLYPH_GAMEPAD_BTN_A},
        entry{VPAD_BUTTON_B, "VPAD_BUTTON_B", CAFE_GLYPH_GAMEPAD_BTN_B},
        entry{VPAD_BUTTON_X, "VPAD_BUTTON_X", CAFE_GLYPH_GAMEPAD_BTN_X},
        entry{VPAD_BUTTON_Y, "VPAD_BUTTON_Y", CAFE_GLYPH_GAMEPAD_BTN_Y},
        entry{VPAD_BUTTON_PLUS, "VPAD_BUTTON_PLUS", CAFE_GLYPH_GAMEPAD_BTN_PLUS},
        entry{VPAD_BUTTON_MINUS, "VPAD_BUTTON_MINUS", CAFE_GLYPH_GAMEPAD_BTN_MINUS},
        entry{VPAD_BUTTON_STICK_L, "VPAD_BUTTON_STICK_L", CAFE_GLYPH_GAMEPAD_BTN_STICK_L},
        entry{VPAD_BUTTON_STICK_R, "VPAD_BUTTON_STICK_R", CAFE_GLYPH_GAMEPAD_BTN_STICK_R},
        entry{VPAD_BUTTON_HOME, "VPAD_BUTTON_HOME", CAFE_GLYPH_GAMEPAD_BTN_HOME},
        entry{VPAD_BUTTON_TV, "VPAD_BUTTON_TV", CAFE_GLYPH_GAMEPAD_BTN_TV},
        entry{VPAD_BUTTON_SYNC, "VPAD_BUTTON_SYNC", "SYNC"},

        entry{VPAD_STICK_L_EMULATION_UP, "VPAD_STICK_L_EMULATION_UP",
              CAFE_GLYPH_GAMEPAD_STICK_L CAFE_GLYPH_ARROW_UP},
        entry{VPAD_STICK_L_EMULATION_DOWN, "VPAD_STICK_L_EMULATION_DOWN",
              CAFE_GLYPH_GAMEPAD_STICK_L CAFE_GLYPH_ARROW_DOWN},
        entry{VPAD_STICK_L_EMULATION_LEFT, "VPAD_STICK_L_EMULATION_LEFT",
              CAFE_GLYPH_GAMEPAD_STICK_L CAFE_GLYPH_ARROW_LEFT},
        entry{VPAD_STICK_L_EMULATION_RIGHT, "VPAD_STICK_L_EMULATION_RIGHT",
              CAFE_GLYPH_GAMEPAD_STICK_L CAFE_GLYPH_ARROW_RIGHT},

        entry{VPAD_STICK_R_EMULATION_UP, "VPAD_STICK_R_EMULATION_UP",
              CAFE_GLYPH_GAMEPAD_STICK_R CAFE_GLYPH_ARROW_UP},
        entry{VPAD_STICK_R_EMULATION_DOWN, "VPAD_STICK_R_EMULATION_DOWN",
              CAFE_GLYPH_GAMEPAD_STICK_R CAFE_GLYPH_ARROW_DOWN},
        entry{VPAD_STICK_R_EMULATION_LEFT, "VPAD_STICK_R_EMULATION_LEFT",
              CAFE_GLYPH_GAMEPAD_STICK_R CAFE_GLYPH_ARROW_LEFT},
        entry{VPAD_STICK_R_EMULATION_RIGHT, "VPAD_STICK_R_EMULATION_RIGHT",
              CAFE_GLYPH_GAMEPAD_STICK_R CAFE_GLYPH_ARROW_RIGHT},
};

std::string
concat(const std::string &a,
       const std::string &b,
       const char *sep = "") {
    if (a.empty())
        return b;
    if (b.empty())
        return a;
    return a + sep + b;
}


std::string
to_glyph(const uint32_t buttons) {
    std::string result;
    for (auto [button, name, glyph] : button_entries)
        if (button & buttons)
            result = concat(result, glyph);
    return result;
}


static uint32_t sWPADLastButtonHold[4] = {0, 0, 0, 0};


DECL_FUNCTION(void, WPADRead, WPADChan chan, WPADStatusProController *data) {
    real_WPADRead(chan, data);

    if (data && chan >= 0 && chan < 4) {
        if (data[0].err == 0) {
            if (data[0].extensionType != 0xFF) {
                uint32_t curButtonHold = 0;
                if (data[0].extensionType == WPAD_EXT_CORE || data[0].extensionType == WPAD_EXT_NUNCHUK) {
                    // button data is in the first 2 bytes for wiimotes
                    curButtonHold = remapWiiMoteButtons(((uint16_t *) data)[0]);
                } else if (data[0].extensionType == WPAD_EXT_CLASSIC) {
                    curButtonHold = remapClassicButtons(((uint32_t *) data)[10] & 0xFFFF);
                } else if (data[0].extensionType == WPAD_EXT_PRO_CONTROLLER) {
                    curButtonHold = remapProButtons(data[0].buttons);
                }

                const uint32_t curButtonTrigger = (curButtonHold & (~(sWPADLastButtonHold[chan])));

                if (!sRestartTriggered && (curButtonTrigger & VPAD_BUTTON_L)) {
                    if (!sIsPlayback) {
                        sDoStartPlayback = true;
                        _SYSLaunchTitleWithStdArgsInNoSplash(OSGetTitleID(), nullptr);
                        sRestartTriggered = true;
                    }
                }
                if (!sRestartTriggered && (curButtonTrigger & VPAD_BUTTON_STICK_L)) {
                    if (!sIsRecording && !sIsPlayback) {
                        sDoStartRecording = true;
                        _SYSLaunchTitleWithStdArgsInNoSplash(OSGetTitleID(), nullptr);
                        sRestartTriggered = true;
                    }
                }
                if (curButtonTrigger & VPAD_BUTTON_STICK_R) {
                    if (sIsRecording) {
                        sIsRecording = false;
                        if (sRecordNotification != 0) {
                            NotificationModule_UpdateDynamicNotificationText(sRecordNotification, "Recording done");
                            NotificationModule_FinishDynamicNotification(sRecordNotification, 2);
                            sRecordNotification = 0;
                        }
                    } else if (sIsPlayback) {
                        sIsPlayback = false;
                        if (sPlaybackNotification != 0) {
                            NotificationModule_UpdateDynamicNotificationText(sPlaybackNotification, "Playback aborted");
                            NotificationModule_FinishDynamicNotification(sPlaybackNotification, 2);
                            sPlaybackNotification = 0;
                        }
                    }
                }
            }
        }
    }
}

DECL_FUNCTION(int32_t, VPADRead, VPADChan chan,
              VPADStatus *buffers,
              uint32_t count,
              VPADReadError *outError) {
    if (sIsPlayback && chan == VPAD_CHAN_0) {
        if (sPlaybackOffset < controllerData.size()) {
            memcpy(buffers, &controllerData[sPlaybackOffset].data, sizeof(VPADStatus));
            if (sPlaybackOffset < controllerData.size()) {
                const auto txt = string_format("Playback frame %05d. Total frames %05d: %s", sPlaybackOffset + 1, controllerData.size(), to_glyph(controllerData[sPlaybackOffset].data.hold).c_str());
                NotificationModule_UpdateDynamicNotificationText(sPlaybackNotification, txt.c_str());
            }
            sPlaybackOffset++;
            if (outError) {
                *outError = controllerData[sPlaybackOffset].error;
            }
            return controllerData[sPlaybackOffset].result;
        } else {
            const auto txt = string_format("Playback done. Total frames %05d: %s", controllerData.size());
            NotificationModule_UpdateDynamicNotificationText(sPlaybackNotification, txt.c_str());
            NotificationModule_FinishDynamicNotification(sPlaybackNotification, 2);
            sPlaybackNotification = 0;
            sIsPlayback           = false;
        }
    }
    VPADReadError real_error;
    const auto result = real_VPADRead(chan, buffers, count, &real_error);
    if (sIsRecording && chan == VPAD_CHAN_0) {
        if (buffers) {
            controllerData.emplace_back(result > 0 ? 1 : result, *buffers, real_error);
        } else {
            controllerData.push_back({result, {}, real_error});
        }
        if (result == 0 || real_error != VPAD_READ_SUCCESS) {
            DEBUG_FUNCTION_LINE_ERR("WE HAD AN READ ERROR!!");
        }
        if (sRecordNotification != 0) {
            const auto txt = string_format("Recording frame %05d: %s", controllerData.size(), to_glyph(controllerData.back().data.hold).c_str());
            NotificationModule_UpdateDynamicNotificationText(sRecordNotification, txt.c_str());
        }
    }
    if (outError) {
        *outError = real_error;
    }
    return result > 0 ? 1 : result;
}

WUPS_MUST_REPLACE(VPADRead, WUPS_LOADER_LIBRARY_VPAD, VPADRead);
WUPS_MUST_REPLACE(WPADRead, WUPS_LOADER_LIBRARY_PADSCORE, WPADRead);
