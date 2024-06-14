#include <iostream>
#include "settings.h"
#include <SDL.h>

void showGamepadSettings() {
    std::cout << "Gamepad-Einstellungen:" << std::endl;
    for (const auto& binding : gamepadBindings) {
        std::cout << "Taste " << binding.first << " => Aktion: " << binding.second << std::endl;
    }
}

void adjustUIForHiDPI(SDL_Renderer* renderer) {
    float dpi;
    if (SDL_GetDisplayDPI(0, NULL, &dpi, NULL) != 0) {
        std::cerr << "Fehler beim Abrufen des DPI-Werts: " << SDL_GetError() << std::endl;
        return;
    }
    float scale = dpi / 96.0f;

    // Skalieren der UI-Elemente basierend auf DPI
    // Beispiel: Skalierung eines Buttons
    int buttonWidth = static_cast<int>(200 * scale);
    int buttonHeight = static_cast<int>(50 * scale);

    std::cout << "Skalierte Button-Größe: " << buttonWidth << "x" << buttonHeight << std::endl;
    // Erstellen oder Anpassen der UI-Elemente hier
    // Beispiel: Zeichnen eines skalierbaren Buttons
    SDL_Rect buttonRect = { 100, 100, buttonWidth, buttonHeight };
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &buttonRect);
    SDL_RenderPresent(renderer);
}
