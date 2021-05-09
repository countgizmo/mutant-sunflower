#include "ui.h"

void 
button(SDL_Surface *target_surface, TTF_Font *font, int x, int y, char *text)
{
    int text_width;
    int text_height;
    TTF_SizeText(font, text, &text_width, &text_height);

    int button_width = text_width + 10;
    int button_height = text_height + 4;

    SDL_Rect destination = {};
    destination.x = x;
    destination.y = y;
    destination.w = button_width;
    destination.h = button_height;

    SDL_Surface *button_surface = SDL_CreateRGBSurface(0, button_width, button_height, 32, 0, 0, 0, 0);
    SDL_FillRect(button_surface, NULL, SDL_MapRGB(button_surface->format, 255, 99, 99));

    SDL_Color color = {0, 2, 114};
    SDL_Surface *text_surface = TTF_RenderText_Solid(font, text, color);

    SDL_Rect text_destination = {};
    text_destination.x = (button_width / 2) - (text_width / 2);
    text_destination.y = (button_height / 2) - (text_height / 2);
    text_destination.w = text_width;
    text_destination.h = text_height;

    SDL_BlitSurface(text_surface, NULL, button_surface, &text_destination);
    SDL_BlitSurface(button_surface, NULL, target_surface, &destination);

    SDL_FreeSurface(text_surface);
    SDL_FreeSurface(button_surface);
}