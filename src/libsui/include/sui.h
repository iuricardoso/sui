#ifndef SUI_H
#define SUI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Tipo opaco que representa a interface de usuário (janela/painel) */
typedef struct SUI_t* SUI;

/* Assinatura do callback do botão (agora recebe apenas o SUI) */
typedef int (*sui_callback_t)(SUI ui);

/* Funções da API SUI */
SUI sui_new(const char* title);
void sui_valueinput(SUI ui, int id, const char* label);
void sui_valueoutput(SUI ui, int id, const char* label);
void sui_button(SUI ui, int id, const char* label);

/* Modificado para receber o dado de usuário no registro */
void sui_setcallback(SUI ui, int id, sui_callback_t cb);

/* Novas funções para recuperar informações do último callback disparado */
int sui_get_id(SUI ui);
void* sui_get_userdata(SUI ui);

/* Exibe uma caixa de mensagem com formatação de string */
void sui_alert(SUI ui, const char* format, ...);

/* Funções de formatação e leitura inspiradas no stdio */
int sui_scanf(SUI ui, int id, const char* format, ...);
int sui_printf(SUI ui, int id, const char* format, ...);

/* Controle do ciclo de vida */
void sui_show(SUI ui);
int sui_run(SUI* ui_ptr);

#ifdef __cplusplus
}
#endif

#endif // SUI_H
