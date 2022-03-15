#include "menu.h"
#include "titles.h"
#include "users.h"
#include "navigation.h"
#include "titlelist.h"
#include "dumping.h"
#include "filesystem.h"
#include "gui.h"
#include "iosuhax.h"
#include "../font/log_freetype.h"

// Menu screens

void showLoadingScreen() {
    WHBLogFreetypeSetBackgroundColor(0x0b5d5e00);
    WHBLogFreetypeSetFontColor(0xFFFFFFFF);
    WHBLogFreetypeSetFontSize(22, 0);
    WHBLogPrint("Dumpling V2.2.4");
    WHBLogPrint("-- Made by Crementif and Emiyl --");
    WHBLogPrint("");
    WHBLogFreetypeDraw();
}

void showMainMenu() {
    uint8_t selectedOption = 0;
    bool startSelectedOption = false;
    while(!startSelectedOption) {
        // Print menu text
        WHBLogFreetypeStartScreen();
        WHBLogPrint("Dumpling V2.2.4");
        WHBLogPrint("===============================");
        WHBLogPrintf("%c Dump a game disc", selectedOption==0 ? '>' : ' ');
        WHBLogPrintf("%c Dump digital games", selectedOption==1 ? '>' : ' ');
        WHBLogPrint("");
        WHBLogPrintf("%c Dump files to use Cemu online", selectedOption==2 ? '>' : ' ');
        WHBLogPrintf("%c Dump Wii U applications (e.g. Friend List, eShop etc.)", selectedOption==3 ? '>' : ' ');
        //WHBLogPrintf("%c Dump Amiibo Files for Cemu", selectedOption==4 ? '>' : ' ');
        WHBLogPrint("");
        WHBLogPrintf("%c Dump only Base files of a game", selectedOption==4 ? '>' : ' ');
        WHBLogPrintf("%c Dump only Update files of a game", selectedOption==5 ? '>' : ' ');
        WHBLogPrintf("%c Dump only DLC files of a game", selectedOption==6 ? '>' : ' ');
        WHBLogPrintf("%c Dump whole MLC (everything stored on internal storage)", selectedOption==7 ? '>' : ' ');
        WHBLogFreetypeScreenPrintBottom("===============================");
        WHBLogFreetypeScreenPrintBottom("\uE000 Button = Select Option");
        WHBLogFreetypeScreenPrintBottom("\uE001 Button = Exit Dumpling");
        WHBLogFreetypeDrawScreen();

        // Loop until there's new input
        sleep_for(200ms); // Cooldown between each button press
        updateInputs();
        while(!startSelectedOption) {
            updateInputs();
            // Check each button state
            if (navigatedUp() && selectedOption > 0) {
                selectedOption--;
                break;
            }
            if (navigatedDown() && selectedOption < 8) {
                selectedOption++;
                break;
            }
            if (pressedOk()) {
                startSelectedOption = true;
                break;
            }
            if (pressedBack()) {
                uint8_t exitSelectedOption = showDialogPrompt(getCFWVersion() == TIRAMISU_RPX ? "Do you really want to exit Dumpling?" : "Do you really want to exit Dumpling?\nYour console will shutdown to prevent compatibility issues!", "Yes", "No");
                if (exitSelectedOption == 0) {
                    WHBLogFreetypeClear();
                    return;
                }
                else break;
            }
            sleep_for(50ms);
        }
    }

    // Go to the selected menu
    switch(selectedOption) {
        case 0:
            dumpDisc();
            break;
        case 1:
            showTitleList("Select all the games you want to dump!", {.filterTypes = dumpTypeFlags::GAME, .dumpTypes = (dumpTypeFlags::GAME | dumpTypeFlags::UPDATE | dumpTypeFlags::DLC | dumpTypeFlags::SAVE), .queue = true});
            break;
        case 2:
            dumpOnlineFiles();
            break;
        case 3:
            showTitleList("Select all the system applications you want to dump!", {.filterTypes = dumpTypeFlags::SYSTEM_APP, .dumpTypes = dumpTypeFlags::GAME, .queue = true});
            break;
        case 4:
            showTitleList("Select all the games that you want to dump the base game from!", {.filterTypes = dumpTypeFlags::GAME, .dumpTypes = dumpTypeFlags::GAME, .queue = true});
            break;
        case 5:
            showTitleList("Select all the games that you want to dump the update from!", {.filterTypes = dumpTypeFlags::UPDATE, .dumpTypes = dumpTypeFlags::UPDATE, .queue = true});
            break;
        case 6:
            showTitleList("Select all the games that you want to dump the DLC from!", {.filterTypes = dumpTypeFlags::DLC, .dumpTypes = dumpTypeFlags::DLC, .queue = true});
            break;
        case 7:
            dumpMLC();
            break;
        case 8:
            break;
        case 9:
            //showTitleList("Select all the games that you want to dump the save from!", {.filterTypes = (dumpTypeFlags::SAVE | dumpTypeFlags::COMMONSAVE), .dumpTypes = (dumpTypeFlags::SAVE | dumpTypeFlags::COMMONSAVE), .queue = true});
            break;
        default:
            break;
    }

    cleanDumpingProcess();

    sleep_for(500ms);
    showMainMenu();
}

bool showOptionMenu(dumpingConfig& config, bool showAccountOption) {
    uint8_t selectedOption = 0;
    uint8_t selectedAccount = 0;

    while(!(isSDInserted() || isUSBDriveInserted())) {
        WHBLogFreetypeStartScreen();
        WHBLogPrint("Couldn't detect an SD card or USB drive!");
        WHBLogPrint("");
        WHBLogPrint("If you do have one inserted, you could try:");
        WHBLogPrint(" - Reinserting the SD card or USB drive");
        WHBLogPrint(" - Make sure it's formatted as FAT32");
        WHBLogPrint(" - It only has one partition (and no hidden ones)");
        WHBLogPrint(" - Save a Mii picture in Mii Maker to your SD card");
        WHBLogPrint(" - Try a different SD card (recommended) or USB drive");
        WHBLogPrint("");
        WHBLogPrint("If none of those steps worked ask for help on the");
        WHBLogPrint("Cemu discord or report the issue on the Dumpling github.");
        WHBLogFreetypeScreenPrintBottom("===============================");
        WHBLogFreetypeScreenPrintBottom("\uE001 Button = Cancel");
        WHBLogFreetypeDrawScreen();
        sleep_for(100ms);
        updateInputs();
        if (pressedBack()) {
            return false;
        }
    }

    config.location = dumpLocation::SDFat;
    if (!mountSD()) config.location = dumpLocation::USBFat;

    // Detect when multiple online files are getting dumped
    if (showAccountOption && (config.dumpTypes & dumpTypeFlags::CUSTOM) == dumpTypeFlags::CUSTOM && dirExist((getRootFromLocation(config.location)+"/dumpling/Online Files/mlc01/usr/save/system/act/80000001").c_str())) {
        config.dumpAsDefaultUser = false;
    }

    while(true) {
        // Print option menu text
        WHBLogFreetypeStartScreen();
        WHBLogPrint("Change any options for this dump:");
        WHBLogPrint("===============================");
        WHBLogPrintf("%c Dump destination: %s", selectedOption==0 ? '>' : ' ', config.location == dumpLocation::SDFat ? "SD Card" : "USB Drive");
        if (showAccountOption) WHBLogPrintf("%c Account: %s", selectedOption==1 ? '>' : ' ', allUsers[selectedAccount].miiName.c_str());
        if (showAccountOption) WHBLogPrintf("%c Dump Saves/Account For Default Cemu User: %s", selectedOption==2 ? '>' : ' ', config.dumpAsDefaultUser ? "Yes" : "No");
        WHBLogPrint("");
        WHBLogPrintf("%c Start", selectedOption==(1+showAccountOption+showAccountOption) ? '>' : ' ');
        WHBLogFreetypeScreenPrintBottom("===============================");
        WHBLogFreetypeScreenPrintBottom("\uE000 Button = Select Option");
        WHBLogFreetypeScreenPrintBottom("\uE001 Button = Go Back");
        WHBLogFreetypeScreenPrintBottom("\uE07E/\uE081 = Change Value");
        WHBLogFreetypeDrawScreen();

        sleep_for(200ms); // Cooldown between each button press
        updateInputs();
        while(true) {
            updateInputs();
            // Check each button state
            if (navigatedUp() && selectedOption > 0) {
                selectedOption--;
                break;
            }
            if (navigatedDown() && selectedOption < (1+showAccountOption+showAccountOption)) {
                selectedOption++;
                break;
            }
            if (navigatedLeft()) {
                if (selectedOption == 0 && config.location == dumpLocation::USBFat) {
                    if (mountSD()) {
                        config.location = dumpLocation::SDFat;
                        unmountUSBDrive();
                    }
                    else showDialogPrompt("Couldn't detect an useable FAT32 SD card.\nTry reformatting it and make sure it has only one partition.", "OK");
                }
                if (selectedOption == 0 && config.location == dumpLocation::SDFat) {
                    if (mountUSBDrive()) {
                        config.location = dumpLocation::USBFat;
                        unmountSD();
                    }
                    else showDialogPrompt("Couldn't detect an useable FAT32 USB stick.\nTry reformatting it and make sure it has only one partition.", "OK");
                }
                if (selectedOption == 1 && showAccountOption && selectedAccount > 0) {
                    selectedAccount--;
                }
                if (selectedOption == 2 && showAccountOption) {
                    config.dumpAsDefaultUser = !config.dumpAsDefaultUser;
                }
                break;
            }
            if (navigatedRight()) {
                if (selectedOption == 0 && config.location == dumpLocation::USBFat) {
                    if (mountSD()) {
                        config.location = dumpLocation::SDFat;
                        unmountUSBDrive();
                    }
                    else showDialogPrompt("Couldn't detect an useable FAT32 SD card.\nTry reformatting it and make sure it has only one partition.", "OK");
                }
                if (selectedOption == 0 && config.location == dumpLocation::SDFat) {
                    if (mountUSBDrive()) {
                        config.location = dumpLocation::USBFat;
                        unmountSD();
                    }
                    else showDialogPrompt("Couldn't detect an useable FAT32 USB stick.\nTry reformatting it and make sure it has only one partition.", "OK");
                }
                if (selectedOption == 1 && showAccountOption && selectedAccount < allUsers.size()-1) {
                    selectedAccount++;
                }
                if (selectedOption == 2 && showAccountOption) {
                    config.dumpAsDefaultUser = !config.dumpAsDefaultUser;
                }
                break;
            }
            if (pressedOk() && selectedOption == (1+showAccountOption+showAccountOption)) {
                config.accountID = allUsers[selectedAccount].persistentId;
                return true;
            }
            if (pressedBack()) {
                return false;
            }
            sleep_for(50ms);
        }
    }
}


// Helper functions

uint8_t showDialogPrompt(const char* message, const char* button1, const char* button2) {
    sleep_for(100ms);
    uint8_t selectedButton = 0;
    while(true) {
        WHBLogFreetypeStartScreen();

        // Print each line
        std::istringstream messageStream(message);
        std::string line;

        while(std::getline(messageStream, line)) {
            WHBLogPrint(line.c_str());
        }

        WHBLogPrint("");
        WHBLogPrintf("%c %s", selectedButton==0 ? '>' : ' ', button1);
        if (button2 != NULL) WHBLogPrintf("%c %s", selectedButton==1 ? '>' : ' ', button2);
        WHBLogPrint("");
        WHBLogFreetypeScreenPrintBottom("===============================");
        WHBLogFreetypeScreenPrintBottom("\uE000 Button = Select Option");
        WHBLogFreetypeDrawScreen();

        // Input loop
        sleep_for(400ms);
        updateInputs();
        while (true) {
            updateInputs();
            // Handle navigation between the buttons
            if (button2 != NULL) {
                if (navigatedUp() && selectedButton == 1) {
                    selectedButton = 0;
                    break;
                }
                else if (navigatedDown() && selectedButton == 0) {
                    selectedButton = 1;
                    break;
                }
            }

            if (pressedOk()) {
                return selectedButton;
            }

            sleep_for(50ms);
        }
    }
}

void showDialogPrompt(const char* message, const char* button) {
    showDialogPrompt(message, button, NULL);
}

const char* errorMessage = nullptr;
void setErrorPrompt(const char* message) {
    errorMessage = message;
}

void showErrorPrompt(const char* button) {
    std::string promptMessage("An error occured:\n");
    promptMessage += errorMessage;
    showDialogPrompt(promptMessage.c_str(), button);
}