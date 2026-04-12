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

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800
#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

void error_callback(int error, const char* description) {
    fprintf(stderr, "Errore GLFW (%d): %s\n", error, description);
}

void set_style(struct nk_context *ctx) {
    struct nk_color background = nk_rgb(3, 3, 3);
    struct nk_color foreground = nk_rgb(205, 214, 244);
    struct nk_color accent     = nk_rgb(145, 151, 255); 
    struct nk_color selection  = nk_rgb(88, 91, 112);

    struct nk_style *s = &ctx->style;
    s->window.background = background;
    s->window.fixed_background = nk_style_item_color(background);
    s->window.border_color = selection;
    s->text.color = foreground;
    s->button.normal = nk_style_item_color(nk_rgb(31, 31, 31));
    s->button.hover = nk_style_item_color(selection);
    s->button.active = nk_style_item_color(accent);
    s->button.text_normal = foreground;
}

int main(void) {
    struct nk_glfw glfw = {0};
    struct nk_context *ctx;
    GLFWwindow *win;
    
    glfwSetErrorCallback(error_callback);

    printf("[MDC] Inizializzazione GLFW...\n");
    
    /* SUGGERIMENTI PER IL COMPILATORE E IL SISTEMA */
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11); // Forza X11 per compatibilità GLEW

    if (!glfwInit()) {
        fprintf(stderr, "[ERRORE] Impossibile inizializzare GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    printf("[MDC] Creazione finestra...\n");
    win = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "MDC - Minimalist Desktop C-Notes", NULL, NULL);
    if (!win) {
        fprintf(stderr, "[ERRORE] Creazione finestra fallita.\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(win);

    printf("[MDC] Inizializzazione GLEW...\n");
    glewExperimental = GL_TRUE;
    GLenum err = glewInit(); // <--- DICHIARATA CORRETTAMENTE QUI
    
    if (err != GLEW_OK) {
        const char* errMsg = (const char*)glewGetErrorString(err);
        if (strcmp(errMsg, "Unknown error") != 0) {
            fprintf(stderr, "[ERRORE] Inizializzazione GLEW fallita: %s\n", errMsg);
            return -1;
        }
        printf("[MDC] GLEW avverte 'Unknown error', ma ignoriamo su Core Profile.\n");
    }

    ctx = nk_glfw3_init(&glfw, win, NK_GLFW3_INSTALL_CALLBACKS);
    struct nk_font_atlas *atlas;
    nk_glfw3_font_stash_begin(&glfw, &atlas);
    nk_glfw3_font_stash_end(&glfw);

    set_style(ctx); 
    printf("[MDC] Pronto! Avvio loop principale.\n");

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        nk_glfw3_new_frame(&glfw);

        if (nk_begin(ctx, "MDC Editor", nk_rect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT), NK_WINDOW_NO_SCROLLBAR)) {
            nk_layout_row_dynamic(ctx, 30, 1);
            nk_label(ctx, "MDC v0.0.1 - Benvenuto sulla nuova Arch!", NK_TEXT_CENTERED);
            nk_layout_row_dynamic(ctx, 400, 1);
            nk_label(ctx, "Se vedi questa scritta, tutto funziona alla grande.", NK_TEXT_LEFT);
        }
        nk_end(ctx);

        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.01f, 0.01f, 0.01f, 1.0f); 
        nk_glfw3_render(&glfw, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
        glfwSwapBuffers(win);
    }

    nk_glfw3_shutdown(&glfw);
    glfwTerminate();
    return 0;
}