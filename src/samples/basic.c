#include <sui.h>

/*  Define os IDs para cada componente da interface gráfica de usuário: inN1 e
    inN2 são os ids dos campos de entrada numérica. btSomar é o id do botão e
    outResultado é o id do campo de saída.
*/

enum {inN1, inN2, btSomar, outResultado};

/* callback do botão somar, em que recebe por parâmetro a ui (painel ou janela)
    que contém o componente.
*/
int click(SUI ui) {
    float n1, n2, soma;

    sui_scanf(ui, inN1, "%f", &n1);
    sui_scanf(ui, inN2, "%f", &n2);

    soma = n1 + n2;

    sui_printf(ui, outResultado, "%0.2f", soma);

    return 0;
}

int main() {
    // Cria uma janela gráfica com título.
    SUI ui = sui_new("Soma de dois números");

    // Cria dois campos Input.
    sui_valueinput(ui, inN1, "Primeiro número:");
    sui_valueinput(ui, inN2, "Segundo número:");

    // Cria um botão e configura o callback.
    sui_button(ui, btSomar, "Somar");
    sui_setcallback(ui, btSomar, click);

    // Cria um campo de saída.
    sui_valueoutput(ui, outResultado, "Resultado:");

    // Apresenta a janela
    sui_show(ui);

    // Executa o loop da janela.
    return sui_run(&ui);
}


