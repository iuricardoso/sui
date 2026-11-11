#include <oosui.hpp>
#include <string>

using namespace oosui;


struct AppContext {
    Table* pTable;
    Label* pOutCol1;
    Label* pOutCol2;
};

void cbCalc(Component& sender, void* userData) {
    AppContext * ctx = (AppContext*)(userData);
    double sum1 = 0.0;
    double sum2 = 0.0;

    for (int r = 0; r < ctx->pTable->getRowCount(); ++r) {
        try {
            std::string val1 = ctx->pTable->getCellText(r, 0);
            if (!val1.empty()) sum1 += std::stod(val1);
        } catch (...) {}

        try {
            std::string val2 = ctx->pTable->getCellText(r, 1);
            if (!val2.empty()) sum2 += std::stod(val2);
        } catch (...) {}
    }

    ctx->pOutCol1->setText("Soma Col 1: " + std::to_string(sum1));
    ctx->pOutCol2->setText("Soma Col 2: " + std::to_string(sum2));
}

int main() {
    AppContext ctx;

    Window win("Calculadora de Tabela OOSUI");

    Table table;
    table.addColumn("Coluna 1");
    table.addColumn("Coluna 2");

    for (int i = 0; i < 10; ++i) {
        table.addRow(std::to_string(i + 1));
        table.setCellText(i, 0, "5");
        table.setCellText(i, 1, "15");
    }

    Button btnCalc("Calcular Soma");
    Label outCol1("Soma Col 1: 0");
    Label outCol2("Soma Col 2: 0");


    ctx.pTable = &table;
    ctx.pOutCol1 = &outCol1;
    ctx.pOutCol2 = &outCol2;

    btnCalc.setCallback(cbCalc, &ctx);

    win.show();
    win.runEventLoop();

    return 0;
}
