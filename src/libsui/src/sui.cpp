#include "sui.h"
#include "oosui.hpp"

#include <map>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdarg>

/* Estrutura para repassar o callback do botão de C++ para C */
struct ButtonContext {
    SUI ui;
    int id;
    sui_callback_t cb;
    void* user_data; // Armazena o dado de usuário passado no registro
};

/* Definição interna do tipo opaco SUI_t */
struct SUI_t {
    oosui::Window* window;
    std::map<int, oosui::Component*> components;
    std::vector<ButtonContext*> btn_contexts;

    // Estado retido do último callback disparado
    int last_id;
    void* last_userdata;
};

extern "C" {

SUI sui_new(const char* title) {
    SUI ui = new SUI_t;
    ui->window = new oosui::Window(title);
    ui->last_id = -1;
    ui->last_userdata = nullptr;
    return ui;
}

void sui_valueinput(SUI ui, int id, const char* label) {
    if (!ui) return;
    oosui::Input* input = new oosui::Input(label);
    ui->components[id] = input;
    ui->window->addComponent(input);
}

void sui_valueoutput(SUI ui, int id, const char* label) {
    if (!ui) return;
    oosui::Output* output = new oosui::Output(label);
    ui->components[id] = output;
    ui->window->addComponent(output);
}

void sui_button(SUI ui, int id, const char* label) {
    if (!ui) return;
    oosui::Button* btn = new oosui::Button(label);
    ui->components[id] = btn;
    ui->window->addComponent(btn);
}

/* Callback interno C++ que aciona o callback em C do usuário */
static void internal_btn_cb(oosui::Component& sender, void* userData) {
    ButtonContext* ctx = static_cast<ButtonContext*>(userData);
    if (ctx && ctx->cb) {
        // Atualiza o estado da estrutura SUI com as propriedades do botão que ativou o evento
        ctx->ui->last_id = ctx->id;
        ctx->ui->last_userdata = ctx->user_data;

        // Dispara o callback C
        ctx->cb(ctx->ui);
    }
}

void sui_setcallback(SUI ui, int id, sui_callback_t cb) {
    if (!ui) return;

    oosui::Component* comp = ui->components[id];
    oosui::Button* btn = dynamic_cast<oosui::Button*>(comp);

    if (btn) {
        // Agora armazenamos o ponteiro "data" dentro do contexto do botão
        ButtonContext* ctx = new ButtonContext{ui, id, cb, nullptr};
        ui->btn_contexts.push_back(ctx);
        btn->setCallback(internal_btn_cb, ctx);
    }
}

int sui_get_id(SUI ui) {
    if (!ui) return -1;
    return ui->last_id;
}

void* sui_get_userdata(SUI ui) {
    if (!ui) return nullptr;
    return ui->last_userdata;
}

void sui_alert(SUI ui, const char* format, ...) {
    va_list args;
    va_start(args, format);

    // Calcula o tamanho necessário do buffer
    va_list args_copy;
    va_copy(args_copy, args);
    int len = vsnprintf(nullptr, 0, format, args_copy);
    va_end(args_copy);

    if (len >= 0) {
        std::vector<char> buffer(len + 1);
        vsnprintf(buffer.data(), len + 1, format, args);

        // Obtém o HWND da janela principal se a UI for válida, senão usa NULL
        HWND hwnd = (ui && ui->window) ? ui->window->getHWND() : NULL;

        // Chama a função da oosui, utilizando "Alerta" como título padrão
        oosui::ShowMessage(hwnd, std::string(buffer.data()), "Alerta");
    }

    va_end(args);
}

int sui_scanf(SUI ui, int id, const char* format, ...) {
    if (!ui) return 0;

    oosui::Component* comp = ui->components[id];
    // Cast para Input permite buscar o texto tanto de Inputs normais quanto de Outputs
    oosui::Input* input = dynamic_cast<oosui::Input*>(comp);

    if (input) {
        std::string text = input->getText();

        va_list args;
        va_start(args, format);
        int res = vsscanf(text.c_str(), format, args);
        va_end(args);

        return res;
    }
    return 0;
}

int sui_printf(SUI ui, int id, const char* format, ...) {
    if (!ui) return 0;

    oosui::Component* comp = ui->components[id];
    oosui::Input* input = dynamic_cast<oosui::Input*>(comp);

    if (input) {
        va_list args;
        va_start(args, format);

        // Calcula o tamanho necessário do buffer
        va_list args_copy;
        va_copy(args_copy, args);
        int len = vsnprintf(nullptr, 0, format, args_copy);
        va_end(args_copy);

        if (len >= 0) {
            std::vector<char> buffer(len + 1);
            vsnprintf(buffer.data(), len + 1, format, args);
            input->setText(std::string(buffer.data()));
        }

        va_end(args);
        return len;
    }
    return 0;
}

void sui_show(SUI ui) {
    if (ui && ui->window) {
        ui->window->show();
    }
}

int sui_run(SUI* ui_ptr) {
    if (!ui_ptr || !*ui_ptr) return -1;
    SUI ui = *ui_ptr;

    if (ui->window) {
        ui->window->runEventLoop();
    }

    // Liberações de memória ao fechar a janela
    for (auto const& [id, comp] : ui->components) {
        delete comp;
    }
    for (auto* ctx : ui->btn_contexts) {
        delete ctx;
    }
    delete ui->window;
    delete ui;

    *ui_ptr = nullptr;
    return 0;
}

} // extern "C"
