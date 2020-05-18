#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL2/SDL_image.h>

#define WIDTH 800
#define HEIGHT 640

// Ubuntu dependencies:
// sudo apt-get install libsdl2-2.0-0 libsdl2-dev
// sudo apt-get install libsdl2-image-2.0-0 libsdl2-image-dev
// sudo apt-get install libsdl2-ttf-2.0-0 libsdl2-ttf-dev

typedef struct FileEntry {
    std::string name;
    std::string full_path;
    bool is_directory;
    uint64_t size;
    SDL_Texture *name_texture;
    int texture_width;
    int texture_height;
    int click_area[4];
} FileEntry;

typedef struct Paging {
    int pages;
    int current_page;
    int start;
    int end;
    int page_size;
} Paging;

void initialize_renderer(SDL_Renderer *renderer, TTF_Font **font);
void changeDirectory(std::vector<FileEntry*> &file_list, const char *path, SDL_Renderer *renderer, TTF_Font *font, SDL_Color color, Paging *paging);
std::vector<FileEntry*> loadDirectoryFiles(const char *dirname, Paging *paging);
void createFilenameTextures(std::vector<FileEntry*> &file_list, TTF_Font *font, SDL_Color text_color, SDL_Renderer *renderer);
void renderFiles(SDL_Renderer *renderer, std::vector<FileEntry*> &file_list, Paging *paging);
void renderPaging(SDL_Renderer *renderer, Paging *paging, TTF_Font *font);
void mousePress(int mouse_x, int mouse_y, uint8_t button, SDL_Renderer *renderer, std::vector<FileEntry*> &file_list, TTF_Font *font, SDL_Color color, Paging *paging);
int drawHighlight(int mouse_x, int mouse_y, SDL_Renderer *renderer, std::vector<FileEntry*> &file_list, int highlighted, Paging *paging);
bool fileEntryComparator(const FileEntry *a, const FileEntry *b);
void drawRect(SDL_Renderer *renderer, FileEntry* file, SDL_Color color);

int main(int argc, char **argv)
{
    // initializing SDL as Video
    SDL_Init(SDL_INIT_VIDEO);

    // create window and renderer
    SDL_Renderer *renderer;
    SDL_Window *window;
    SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, 0, &window, &renderer);

    // initialize TTF library
    TTF_Init();

    // initialize font and set erase color
    TTF_Font *font;
    initialize_renderer(renderer, &font);

    Paging *paging = new Paging();
    paging->pages = 0;
    paging->current_page = 0;
    paging->start = 0;
    paging->end = 0;
    paging->page_size = 10;

    // Start in home directory
    std::vector<FileEntry*> file_list;
    char* home_path = getenv("HOME");
    SDL_Color text_color = {68, 67, 73};
    changeDirectory(file_list, home_path, renderer, font, text_color, paging);

    int highlighted = -1;
    int mouse_x = 0;
    int mouse_y = 0;
    SDL_Event event;
    SDL_WaitEvent(&event);

    // rendering loop
    while (event.type != SDL_QUIT)
    {
        switch (event.type)
        {
            case SDL_MOUSEMOTION:
                mouse_x = event.motion.x;
                mouse_y = event.motion.y;
                highlighted = drawHighlight(mouse_x, mouse_y, renderer, file_list, highlighted, paging);
                break;
            case SDL_MOUSEBUTTONDOWN:
                mousePress(mouse_x, mouse_y, event.button.button, renderer, file_list, font, text_color, paging);
                highlighted = -1;
                highlighted = drawHighlight(mouse_x, mouse_y, renderer, file_list, highlighted, paging);
                break;
            default:
                break;
        }
        SDL_WaitEvent(&event);
    }

    // clean up
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void initialize_renderer(SDL_Renderer *renderer, TTF_Font **font){
    // set color of background when erasing frame
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    // open a font
    *font = TTF_OpenFont("resrc/fonts/OpenSans/OpenSans-Semibold.ttf", 17);
    if (*font == NULL)
    {
        fprintf(stderr, "Error: could not find font\n");
    }

}

void changeDirectory(std::vector<FileEntry*> &file_list, const char *path, SDL_Renderer *renderer, TTF_Font *font, SDL_Color color, Paging *paging) {
    // get list of files in directory stored in FileEntry structs
    file_list = loadDirectoryFiles(path, paging);
    // create textures for file name text
    createFilenameTextures(file_list, font, color, renderer);
    // Renders the file icon and name to window
    renderFiles(renderer, file_list, paging);

    renderPaging(renderer, paging, font);
}

std::vector<FileEntry*> loadDirectoryFiles(const char *dirname, Paging *paging) {
    std::vector<FileEntry*> result;

    DIR *dir = opendir(dirname);
    if (dir != NULL)
    {
        struct dirent *entry;
        struct stat info;
        while ((entry = readdir(dir)) != NULL)
        {
//            std::cout << entry->d_name << std::endl;
            if (entry->d_name[0] != '.' || strcmp(entry->d_name, "..") == 0)
            {
                FileEntry *file = new FileEntry();
                file->name = entry->d_name;
                file->full_path = std::string(dirname) + "/" + file->name;
                if (entry->d_type == DT_DIR)
                {
                    file->is_directory = true;
                }
                else
                {
                    file->is_directory = false;
                }
                stat(file->full_path.c_str(), &info);
                file->size = info.st_size;

                result.push_back(file);
            }
        }
    }

    std::sort(result.begin(), result.end(), fileEntryComparator);

    int start = paging->current_page * paging->page_size;
    int end = start + paging->page_size;
    int file_list_size = result.size();
    if(end > file_list_size){
        end = file_list_size;
    }
    paging->start = start;
    paging->end = end;

    return result;
}

void createFilenameTextures(std::vector<FileEntry*> &file_list, TTF_Font *font, SDL_Color text_color, SDL_Renderer *renderer) {
    int i;
    for (i = 0; i < file_list.size(); i++) {
        SDL_Surface *surface = TTF_RenderText_Solid(font, file_list[i]->name.c_str(), text_color);
        file_list[i]->name_texture = SDL_CreateTextureFromSurface(renderer, surface);
        file_list[i]->texture_width = surface->w;
        file_list[i]->texture_height = surface->h;
        SDL_FreeSurface(surface);

        file_list[i]->click_area[0] = 40; // x1
        file_list[i]->click_area[1] = 42 + (i * 50);
        file_list[i]->click_area[2] = 55 + file_list[i]->texture_width; // x2
        file_list[i]->click_area[3] = 40;

    }
}

void renderFiles(SDL_Renderer *renderer, std::vector<FileEntry*> &file_list, Paging *paging) {
    // erase renderer content
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    // draw file / folder icons and names
    SDL_Rect rect;
    int i;
    for (i = paging->start; i < paging->end; i++) {
        // icon
        rect.x = 45;
        rect.y = 45 + (i * 50);
        rect.w = 33;
        rect.h = 33;
        const char *file;
        if (file_list[i]->is_directory) {
            file = "resrc/images/folder.svg";
        }
        else
        {
            file = "resrc/images/file.svg";
        }
        SDL_Texture *texture = IMG_LoadTexture(renderer, file);
        SDL_RenderCopy(renderer, texture, NULL, &rect);

        // text
        rect.x = 85;
        rect.y = 50 + (i * 50);
        rect.w = file_list[i]->texture_width;
        rect.h = file_list[i]->texture_height;
        SDL_RenderCopy(renderer, file_list[i]->name_texture, NULL, &rect);
    }

    // show rendered frame
    SDL_RenderPresent(renderer);
}

void renderPaging(SDL_Renderer *renderer, Paging *paging, TTF_Font *font) {
    SDL_Rect rect;
    // Background
    rect.x = 0;
    rect.y = HEIGHT - 70;
    rect.w = WIDTH;
    rect.h = 70;
    SDL_SetRenderDrawColor(renderer, 96, 93, 83, 255);
    SDL_RenderFillRect(renderer, &rect);

    // text
    SDL_Color text_color = {255, 255, 255};
    std::string text = "Page " + std::to_string(paging->current_page + 1);
    SDL_Surface *surface = TTF_RenderText_Solid(font, text.c_str(), text_color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    rect.x = (WIDTH / 2) - (surface->w / 2);
    rect.y = HEIGHT - 50;
    rect.w = surface->w;
    rect.h = surface->h;

    SDL_RenderCopy(renderer, texture, NULL, &rect);

    // show rendered frame
    SDL_RenderPresent(renderer);
}

void mousePress(int mouse_x, int mouse_y, uint8_t button, SDL_Renderer *renderer, std::vector<FileEntry*> &file_list, TTF_Font *font, SDL_Color color, Paging *paging) {
    int i;
    for (i = paging->start; i < paging->end; i++) {
        if (mouse_x >= file_list[i]->click_area[0] &&
            mouse_x <= file_list[i]->click_area[0] + file_list[i]->click_area[2] &&
            mouse_y >= file_list[i]->click_area[1] &&
            mouse_y <= file_list[i]->click_area[1] + file_list[i]->click_area[3])
        {
            const char *path = file_list[i]->full_path.c_str();
            changeDirectory(file_list, path, renderer, font, color, paging);
            break;
        }
    }
}

int drawHighlight(int mouse_x, int mouse_y, SDL_Renderer *renderer, std::vector<FileEntry*> &file_list, int highlighted, Paging *paging) {
    int i;
    for (i = paging->start; i < paging->end; i++) {
        if (mouse_x >= file_list[i]->click_area[0] &&
            mouse_x <= file_list[i]->click_area[0] + file_list[i]->click_area[2] &&
            mouse_y >= file_list[i]->click_area[1] &&
            mouse_y <= file_list[i]->click_area[1] + file_list[i]->click_area[3])
        {
            if(i != highlighted){
                SDL_Color color = {240, 119, 70};
                drawRect(renderer, file_list[i], color);
            }
            return i;
        }
    }
    if(highlighted != -1){
        for (i = paging->start; i < paging->end; i++) {
            SDL_Color color = {255, 255, 255};
            drawRect(renderer, file_list[i], color);
        }
    }
    return -1;
}

void drawRect(SDL_Renderer *renderer, FileEntry* file, SDL_Color color){
    SDL_Rect rect;
    rect.x = -1;
    rect.y = file->click_area[1];
    rect.w = WIDTH+2;
    rect.h = file->click_area[3];
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);

    SDL_RenderDrawRect(renderer, &rect);
    SDL_RenderPresent(renderer);
}

bool fileEntryComparator(const FileEntry *a, const FileEntry *b)
{
    // smallest comes first
    return a->name < b->name;
}

