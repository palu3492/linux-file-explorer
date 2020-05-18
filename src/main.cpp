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

typedef struct FileImages {
    SDL_Texture *file_texture;
    SDL_Texture *directory_texture;
} FileImages;

typedef struct Paging {
    int pages;
    int current_page;
    int start;
    int end;
    int page_size;
    int next_click_area[4];
    int prev_click_area[4];
    SDL_Texture *next_texture;
    SDL_Texture *prev_texture;
    int next_texture_width;
    int next_texture_height;
    int prev_texture_width;
    int prev_texture_height;
    int number_of_files;
} Paging;

void initialize_renderer(SDL_Renderer *renderer, TTF_Font **font);
void changeDirectory(std::vector<FileEntry*> &file_list, const char *path, SDL_Renderer *renderer, TTF_Font *font, SDL_Color color, Paging *paging, FileImages *fileImages);
std::vector<FileEntry*> loadDirectoryFiles(const char *dirname, Paging *paging);
void changePageInfo(Paging *paging);
void createFilenameTextures(std::vector<FileEntry*> &file_list, TTF_Font *font, SDL_Color text_color, SDL_Renderer *renderer, Paging *paging);
void createFileImageTextures(SDL_Renderer *renderer, FileImages *fileImages);
void renderFiles(SDL_Renderer *renderer, std::vector<FileEntry*> &file_list, Paging *paging, FileImages *fileImages);
void createPagingButtons(SDL_Renderer *renderer, Paging *paging, TTF_Font *font);
void renderPaging(SDL_Renderer *renderer, Paging *paging, TTF_Font *font);
void mousePress(int mouse_x, int mouse_y, uint8_t button, SDL_Renderer *renderer, std::vector<FileEntry*> &file_list, TTF_Font *font, SDL_Color color, Paging *paging, FileImages *fileImages);
int drawHighlight(int mouse_x, int mouse_y, SDL_Renderer *renderer, std::vector<FileEntry*> &file_list, int highlighted, Paging *paging);
bool fileEntryComparator(const FileEntry *a, const FileEntry *b);
void drawRectFile(SDL_Renderer *renderer, FileEntry* file, SDL_Color color);
void drawRectButton(SDL_Renderer *renderer, int button, SDL_Color color);
void drawRectButton(SDL_Renderer *renderer, int button, SDL_Color color, Paging *paging);

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

    FileImages *fileImages = new FileImages();
    createFileImageTextures(renderer, fileImages);

    // Setup paging
    Paging *paging = new Paging();
    paging->current_page = 0;
    paging->page_size = 10;

    // Create paging button textures
    createPagingButtons(renderer, paging, font);

    // Start in home directory
    std::vector<FileEntry*> file_list;
    char* home_path = getenv("HOME");
    SDL_Color text_color = {68, 67, 73};
    changeDirectory(file_list, home_path, renderer, font, text_color, paging, fileImages);

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
                mousePress(mouse_x, mouse_y, event.button.button, renderer, file_list, font, text_color, paging, fileImages);
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

void changeDirectory(std::vector<FileEntry*> &file_list, const char *path, SDL_Renderer *renderer, TTF_Font *font, SDL_Color color, Paging *paging, FileImages *fileImages) {
    // get list of files in directory stored in FileEntry structs
    file_list = loadDirectoryFiles(path, paging);
    // create textures for file name text
    createFilenameTextures(file_list, font, color, renderer, paging);
    // Renders the file icon and name to window
    renderFiles(renderer, file_list, paging, fileImages);
    // render paging elements
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

    paging->current_page = 0;
    paging->number_of_files = result.size();
    changePageInfo(paging);
    paging->pages = paging->number_of_files / paging->page_size;
    if(paging->number_of_files % paging->page_size != 0){
        paging->pages++;
    }

    return result;
}

void changePageInfo(Paging *paging){
    int start = paging->current_page * paging->page_size;
    int end = start + paging->page_size;

    if(end > paging->number_of_files){
        end = paging->number_of_files;
    }
    paging->start = start;
    paging->end = end;
}

void createFilenameTextures(std::vector<FileEntry*> &file_list, TTF_Font *font, SDL_Color text_color, SDL_Renderer *renderer, Paging *paging) {
    int i;
    for (i = 0; i < file_list.size(); i++) {
        SDL_Surface *surface = TTF_RenderText_Solid(font, file_list[i]->name.c_str(), text_color);
        file_list[i]->name_texture = SDL_CreateTextureFromSurface(renderer, surface);
        file_list[i]->texture_width = surface->w;
        file_list[i]->texture_height = surface->h;
        SDL_FreeSurface(surface);

        file_list[i]->click_area[0] = 0; // x1
        int y = i % paging->page_size;
        file_list[i]->click_area[1] = 42 + (y * 50);
        file_list[i]->click_area[2] = WIDTH; // x2
        file_list[i]->click_area[3] = 40;

    }
}

void createFileImageTextures(SDL_Renderer *renderer, FileImages *fileImages){
    const char *file = "resrc/images/folder.svg";
    SDL_Texture *texture = IMG_LoadTexture(renderer, file);
    fileImages->directory_texture = texture;
    file = "resrc/images/file.svg";
    texture = IMG_LoadTexture(renderer, file);
    fileImages->file_texture = texture;
}

void renderFiles(SDL_Renderer *renderer, std::vector<FileEntry*> &file_list, Paging *paging, FileImages *fileImages) {
    // erase renderer content
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    // draw file / folder icons and names
    SDL_Rect rect;
    int i;
    for (i = paging->start; i < paging->end; i++) {
        // icon
        rect.x = 45;
        int y = i % paging->page_size;
        rect.y = 45 + (y * 50);
        rect.w = 33;
        rect.h = 33;
        const char *file;

        if (file_list[i]->is_directory) {
            SDL_RenderCopy(renderer, fileImages->directory_texture, NULL, &rect);
        } else {
            SDL_RenderCopy(renderer, fileImages->file_texture, NULL, &rect);
        }

        // text
        rect.x = 85;
        y = i % paging->page_size;
        rect.y = 50 + (y * 50);
        rect.w = file_list[i]->texture_width;
        rect.h = file_list[i]->texture_height;
        SDL_RenderCopy(renderer, file_list[i]->name_texture, NULL, &rect);
    }

    // show rendered frame
    SDL_RenderPresent(renderer);
}

void createPagingButtons(SDL_Renderer *renderer, Paging *paging, TTF_Font *font) {
    SDL_Color text_color = {255, 255, 255};
    // Next
    std::string next_name = "Next Page";
    SDL_Surface *surface = TTF_RenderText_Solid(font, next_name.c_str(), text_color);
    paging->next_texture = SDL_CreateTextureFromSurface(renderer, surface);
    paging->next_texture_width = surface->w;
    paging->next_texture_height = surface->h;

    // Prev
    std::string prev_name = "Previous Page";
    surface = TTF_RenderText_Solid(font, prev_name.c_str(), text_color);
    paging->prev_texture = SDL_CreateTextureFromSurface(renderer, surface);
    paging->prev_texture_width = surface->w;
    paging->prev_texture_height= surface->h;

    SDL_FreeSurface(surface);

    // Click areas
    paging->next_click_area[0] = WIDTH - 30 - paging->next_texture_width; // x1
    paging->next_click_area[1] = HEIGHT - 50;
    paging->next_click_area[2] = paging->next_texture_width; // x2
    paging->next_click_area[3] = paging->next_texture_height;

    paging->prev_click_area[0] = 30; // x1
    paging->prev_click_area[1] = HEIGHT - 50;
    paging->prev_click_area[2] = paging->prev_texture_width; // x2
    paging->prev_click_area[3] = paging->prev_texture_height;
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
    std::string text = "Page " + std::to_string(paging->current_page + 1) + "/" + std::to_string(paging->pages);
    SDL_Surface *surface = TTF_RenderText_Solid(font, text.c_str(), text_color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    rect.x = (WIDTH / 2) - (surface->w / 2);
    rect.y = HEIGHT - 50;
    rect.w = surface->w;
    rect.h = surface->h;

    SDL_RenderCopy(renderer, texture, NULL, &rect);

    // Buttons
    rect.x = 30;
    rect.y = HEIGHT - 50;
    rect.w = paging->prev_texture_width;
    rect.h = paging->prev_texture_height;
    SDL_RenderCopy(renderer, paging->prev_texture, NULL, &rect);

    rect.x = WIDTH - 30 - paging->next_texture_width;
    rect.y = HEIGHT - 50;
    rect.w = paging->next_texture_width;
    rect.h = paging->next_texture_height;
    SDL_RenderCopy(renderer, paging->next_texture, NULL, &rect);

    // show rendered frame
    SDL_RenderPresent(renderer);
}

void mousePress(int mouse_x, int mouse_y, uint8_t button, SDL_Renderer *renderer, std::vector<FileEntry*> &file_list, TTF_Font *font, SDL_Color color, Paging *paging, FileImages *fileImages) {
    int i;
    for (i = paging->start; i < paging->end; i++) {
        if (mouse_x >= file_list[i]->click_area[0] &&
            mouse_x <= file_list[i]->click_area[0] + file_list[i]->click_area[2] &&
            mouse_y >= file_list[i]->click_area[1] &&
            mouse_y <= file_list[i]->click_area[1] + file_list[i]->click_area[3])
        {
            if(file_list[i]->is_directory){
                const char *path = file_list[i]->full_path.c_str();
                changeDirectory(file_list, path, renderer, font, color, paging, fileImages);
                break;
            } else {
                std::cout << "Open file" << std::endl;
            }
        }
    }
    if (mouse_x >= paging->next_click_area[0] &&
        mouse_x <= paging->next_click_area[0] + paging->next_click_area[2] &&
        mouse_y >= paging->next_click_area[1] &&
        mouse_y <= paging->next_click_area[1] + paging->next_click_area[3])
    {
        if(paging->current_page < paging->pages - 1) {
            paging->current_page++;
            changePageInfo(paging);
            // Renders the file icon and name to window
            renderFiles(renderer, file_list, paging, fileImages);
            // render paging elements
            renderPaging(renderer, paging, font);
        }
    } else if (mouse_x >= paging->prev_click_area[0] &&
               mouse_x <= paging->prev_click_area[0] + paging->prev_click_area[2] &&
               mouse_y >= paging->prev_click_area[1] &&
               mouse_y <= paging->prev_click_area[1] + paging->prev_click_area[3])
    {
        if(paging->current_page > 0) {
            paging->current_page--;
            changePageInfo(paging);
            // Renders the file icon and name to window
            renderFiles(renderer, file_list, paging, fileImages);
            // render paging elements
            renderPaging(renderer, paging, font);
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
                drawRectFile(renderer, file_list[i], color);
            }
            return i;
        }
    }
    if (mouse_x >= paging->next_click_area[0] &&
        mouse_x <= paging->next_click_area[0] + paging->next_click_area[2] &&
        mouse_y >= paging->next_click_area[1] &&
        mouse_y <= paging->next_click_area[1] + paging->next_click_area[3])
    {
        if(highlighted != -2){
            SDL_Color color = {255, 255, 255};
            drawRectButton(renderer, 0, color, paging);
        }
        return -2;
    } else if (mouse_x >= paging->prev_click_area[0] &&
               mouse_x <= paging->prev_click_area[0] + paging->prev_click_area[2] &&
               mouse_y >= paging->prev_click_area[1] &&
               mouse_y <= paging->prev_click_area[1] + paging->prev_click_area[3])
    {
        if(highlighted != -3){
            SDL_Color color = {255, 255, 255};
            drawRectButton(renderer, 1, color, paging);
        }
        return -3;
    }
    if(highlighted > -1){
        for (i = paging->start; i < paging->end; i++) {
            SDL_Color color = {255, 255, 255};
            drawRectFile(renderer, file_list[i], color);
        }
    } else if(highlighted < -1) {
        SDL_Color color = {96, 93, 83};
        drawRectButton(renderer, 0, color, paging);
        drawRectButton(renderer, 1, color, paging);
    }
    return -1;
}

void drawRectFile(SDL_Renderer *renderer, FileEntry* file, SDL_Color color){
    SDL_Rect rect;
    rect.x = -1;
    rect.y = file->click_area[1];
    rect.w = WIDTH+2;
    rect.h = file->click_area[3];
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);

    SDL_RenderDrawRect(renderer, &rect);
    SDL_RenderPresent(renderer);
}

void drawRectButton(SDL_Renderer *renderer, int button, SDL_Color color, Paging *paging){
    SDL_Rect rect;
    if(button == 0){
        rect.x = paging->next_click_area[0] - 5;
        rect.y = paging->next_click_area[1] - 5;
        rect.w = paging->next_click_area[2] + 10;
        rect.h = paging->next_click_area[3] + 10;
    } else if(button == 1){
        rect.x = paging->prev_click_area[0] - 5;
        rect.y = paging->prev_click_area[1] - 5;
        rect.w = paging->prev_click_area[2] + 10;
        rect.h = paging->prev_click_area[3] + 10;
    }
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
    SDL_RenderDrawRect(renderer, &rect);
    SDL_RenderPresent(renderer);
}

bool fileEntryComparator(const FileEntry *a, const FileEntry *b)
{
    // smallest comes first
    return a->name < b->name;
}

