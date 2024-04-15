void button(SDL_Surface *target_surface, TTF_Font *font, int x, int y, char *text)
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

void
rect(SDL_Surface *target_surface, int x, int y, int w, int h, uint16 border_width, uint32 stroke_color, uint32 fill_color)
{
    SDL_Rect destination = {};
    destination.x = x;
    destination.y = y;
    destination.w = w;
    destination.h = h;

    SDL_Rect outline_area =
    {
        .x = 0,
        .y = 0,
        .w = w,
        .h = h
    };

    SDL_Rect fill_area =
    {
        .x = 0 + border_width,
        .y = 0 + border_width,
        .w = w - (2 * border_width),
        .h = h - (2 * border_width)
    };


    SDL_Surface *rect_surface = SDL_CreateRGBSurface(0, w, h, 32, 0, 0, 0, 0);


    SDL_FillRect(rect_surface, &outline_area, stroke_color);
    SDL_FillRect(rect_surface, &fill_area, fill_color);

    SDL_BlitSurface(rect_surface, NULL, target_surface, &destination);
    SDL_FreeSurface(rect_surface);
}

internal void
copy_text_as_surface(SDL_Surface *dest_surface, const char *text, SDL_Rect text_position, TTF_Font *font, SDL_Color font_color)
{
    SDL_Surface *text_surface = TTF_RenderText_Solid(font, text, font_color);

    if (SDL_BlitSurface(text_surface, NULL, dest_surface, &text_position) < 0)
    {
        printf("(copy_text_as_surface): SDL_BlitSurface failed: %s\n", SDL_GetError());
    }

    SDL_FreeSurface(text_surface);

}
