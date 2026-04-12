#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <time.h>
#include <dirent.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear.h"
#include "nuklear_glfw_gl3.h"
#include "md4c.h"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800
#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

// --- STATO ---
typedef enum { MODE_EDIT, MODE_READ } ViewMode;
ViewMode current_mode = MODE_EDIT;
int is_header = 0; // Per sapere se stiamo scrivendo un titolo in Read Mode

// --- LOGICA FILE ---
#define MAX_FILES 100
char filenames[MAX_FILES][256];
int file_count = 0;
char current_file[256] = "Nessun file selezionato";
char text_buffer[1024 * 64]; 
int text_len = 0;
int prev_text_len = 0;

// --- AUTOSAVE ---
double last_edit_time = 0.0;
int needs_save = 0;
const double AUTOSAVE_DELAY = 1.5;

// --- CALLBACK MD4C CORRETTE ---

int cb_enter_block(MD_BLOCKTYPE type, void* detail, void* userdata) {
    struct nk_context* ctx = (struct nk_context*)userdata;
    
    if (type == MD_BLOCK_H) {
        // MD4C passa i dettagli del titolo qui
        MD_BLOCK_H_DETAIL* h = (MD_BLOCK_H_DETAIL*)detail;
        is_header = 1;
        
        // Possiamo cambiare altezza in base al livello (h->level è 1, 2, 3...)
        if (h->level == 1) nk_layout_row_dynamic(ctx, 40, 1);
        else nk_layout_row_dynamic(ctx, 30, 1);
    } else {
        is_header = 0;
        nk_layout_row_dynamic(ctx, 25, 1);
    }
    return 0;
}

int cb_leave_block(MD_BLOCKTYPE type, void* detail, void* userdata) {
    if (type == MD_BLOCK_H) is_header = 0;
    return 0;
}

// Questa la usiamo per il testo
int cb_text(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata) {
    struct nk_context* ctx = (struct nk_context*)userdata;
    char temp[size + 1];
    memcpy(temp, text, size);
    temp[size] = '\0';

    if (is_header) {
        // Titoli con il tuo colore accento (Lavanda)
        nk_label_colored(ctx, temp, NK_TEXT_LEFT, nk_rgb(145, 151, 255));
    } else {
        // Testo normale
        nk_label(ctx, temp, NK_TEXT_LEFT);
    }
    return 0;
}

// Aggiorna la struttura parser aggiungendo cb_leave_block
MD_PARSER md_parser = { 
    0, 
    MD_FLAG_TABLES | MD_FLAG_UNDERLINE | MD_FLAG_STRIKETHROUGH, 
    cb_enter_block, 
    cb_leave_block, // Aggiunto per resettare is_header correttamente
    NULL, NULL, 
    cb_text, 
    NULL, NULL 
};
// --- FUNZIONI DI SERVIZIO ---
void scan_directory(const char *path) {
    struct dirent *entry;
    DIR *dp = opendir(path);
    file_count = 0;
    if (dp == NULL) return;
    while ((entry = readdir(dp))) {
        if (strstr(entry->d_name, ".md")) {
            strncpy(filenames[file_count], entry->d_name, 255);
            file_count++;
        }
        if (file_count >= MAX_FILES) break;
    }
    closedir(dp);
}

void load_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (size < (long)sizeof(text_buffer)) {
            size_t n = fread(text_buffer, 1, size, f);
            text_buffer[n] = '\0';
            text_len = (int)n;
            prev_text_len = text_len;
            strncpy(current_file, filename, 255);
            needs_save = 0;
        }
        fclose(f);
    }
}

void save_current_file() {
    if (strcmp(current_file, "Nessun file selezionato") == 0) return;
    FILE *f = fopen(current_file, "wb");
    if (f) {
        fwrite(text_buffer, 1, text_len, f);
        fclose(f);
        needs_save = 0;
        printf("[MDC] Salvato: %s\n", current_file);
    }
}

void set_style(struct nk_context *ctx) {
    struct nk_color background = nk_rgb(3, 3, 3);
    struct nk_color foreground = nk_rgb(205, 214, 244);
    struct nk_color accent     = nk_rgb(145, 151, 255); 
    struct nk_color selection  = nk_rgb(58, 59, 112);

    struct nk_style *s = &ctx->style;
    s->window.background = background;
    s->window.fixed_background = nk_style_item_color(background);
    s->window.border_color = selection;
    s->text.color = foreground;
    s->button.normal = nk_style_item_color(nk_rgb(20, 20, 20));
    s->button.hover = nk_style_item_color(selection);
    s->button.active = nk_style_item_color(accent);
    s->button.text_normal = foreground;
}

int main(void) {
    struct nk_glfw glfw = {0};
    struct nk_context *ctx;
    GLFWwindow *win;
    
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    win = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "MDC - Markdown Desktop C", NULL, NULL);
    glfwMakeContextCurrent(win);
    glewExperimental = GL_TRUE;
    glewInit();

    ctx = nk_glfw3_init(&glfw, win, NK_GLFW3_INSTALL_CALLBACKS);
    struct nk_font_atlas *atlas;
    nk_glfw3_font_stash_begin(&glfw, &atlas);
    nk_glfw3_font_stash_end(&glfw);

    set_style(ctx); 
    scan_directory(".");

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        nk_glfw3_new_frame(&glfw);

        int w, h;
        glfwGetWindowSize(win, &w, &h);

        if (nk_begin(ctx, "MDC", nk_rect(0, 0, w, h), NK_WINDOW_NO_SCROLLBAR)) {
            nk_layout_row_begin(ctx, NK_STATIC, h - 30, 2);
            
            // --- SIDEBAR ---
            nk_layout_row_push(ctx, 200);
            if (nk_group_begin(ctx, "Directory", NK_WINDOW_BORDER)) {
                nk_layout_row_dynamic(ctx, 25, 1);
                nk_label(ctx, "DIRECTORY", NK_TEXT_CENTERED);
                
                nk_layout_row_dynamic(ctx, 20, 1);
                if (needs_save) nk_label_colored(ctx, "● Modificato", NK_TEXT_LEFT, nk_rgb(255, 200, 0));
                else nk_label_colored(ctx, "✓ Salvato", NK_TEXT_LEFT, nk_rgb(0, 255, 150));

                nk_layout_row_dynamic(ctx, 30, 1);
                for (int i = 0; i < file_count; i++) {
                    if (nk_button_label(ctx, filenames[i])) {
                        if (needs_save) save_current_file();
                        load_file(filenames[i]);
                    }
                }
                nk_group_end(ctx);
            }

            // --- MAIN AREA ---
            nk_layout_row_push(ctx, w - 230);
            if (nk_group_begin(ctx, "MainContent", NK_WINDOW_NO_SCROLLBAR)) {
                
                nk_layout_row_begin(ctx, NK_DYNAMIC, 30, 2);
                nk_layout_row_push(ctx, 0.7f);
                nk_label(ctx, current_file, NK_TEXT_LEFT);
                
                nk_layout_row_push(ctx, 0.3f);
                if (nk_button_label(ctx, current_mode == MODE_EDIT ? "MODO: LETTURA" : "MODO: SCRITTURA")) {
                    current_mode = (current_mode == MODE_EDIT) ? MODE_READ : MODE_EDIT;
                    if (current_mode == MODE_READ) save_current_file();
                }
                nk_layout_row_end(ctx);

                nk_layout_row_dynamic(ctx, h - 100, 1);
                if (current_mode == MODE_EDIT) {
                    // EDITOR ATTIVO
                    nk_edit_string(ctx, NK_EDIT_BOX, text_buffer, &text_len, sizeof(text_buffer), nk_filter_default);
                    if (text_len != prev_text_len) {
                        last_edit_time = glfwGetTime();
                        needs_save = 1;
                        prev_text_len = text_len;
                    }
                } else {
                    // RENDERER MARKDOWN
                    if (nk_group_begin(ctx, "Reader", NK_WINDOW_BORDER)) {
                        md_parse(text_buffer, text_len, &md_parser, (void*)ctx);
                        nk_group_end(ctx);
                    }
                }
                nk_group_end(ctx);
            }
            nk_layout_row_end(ctx);
        }
        nk_end(ctx);

        if (needs_save && (glfwGetTime() - last_edit_time) > AUTOSAVE_DELAY) {
            save_current_file();
        }

        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.01f, 0.01f, 0.01f, 1.0f); 
        nk_glfw3_render(&glfw, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
        glfwSwapBuffers(win);
    }
    nk_glfw3_shutdown(&glfw);
    glfwTerminate();
    return 0;
}