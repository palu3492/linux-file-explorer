#include <iostream>
#include <SDL.h>

#define WIDTH 800
#define HEIGHT 600

// Ubuntu dependencies:
// sudo apt-get install libsdl2-2.0-0 libsdl2-dev
// sudo apt-get install libsdl2-image-2.0-0 libsdl2-image-dev
// sudo apt-get install libsdl2-ttf-2.0-0 libsdl2-ttf-dev


void initialize(SDL_Renderer *renderer);
void render(SDL_Renderer *renderer);

int main(int argc, char **argv)
{
    // initializing SDL as Video
    SDL_Init(SDL_INIT_VIDEO);

    // create window and renderer
    SDL_Renderer *renderer;
    SDL_Window *window;
    SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, 0, &window, &renderer);

    // initialize and perform rendering loop
    initialize(renderer);
    render(renderer);
    SDL_Event event;
    SDL_WaitEvent(&event);
    while (event.type != SDL_QUIT)
    {
        //render(renderer);
        SDL_WaitEvent(&event);
    }

    // clean up
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void initialize(SDL_Renderer *renderer)
{
    // set color of background when erasing frame
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
}

void render(SDL_Renderer *renderer)
{
    // erase renderer content
    SDL_RenderClear(renderer);
    
    // TODO: draw!

    // show rendered frame
    SDL_RenderPresent(renderer);
}

