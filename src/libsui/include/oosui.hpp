#ifndef OOSUI_HPP
#define OOSUI_HPP

#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <dwmapi.h>
#include <gdiplus.h>
#include <uxtheme.h>
#include <commctrl.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <algorithm>

#ifdef _MSC_VER
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#endif

namespace oosui {

namespace utils {
    std::wstring to_wstring(const std::string& str);
    std::string to_string(const std::wstring& wstr);
}

void ShowMessage(HWND hwnd, const std::string& msg, const std::string& title);
void ShowMessage(HWND hwnd, const std::wstring& msg, const std::wstring& title);

class Panel;

class GDIPlusManager {
private:
    ULONG_PTR token = 0;
    bool active = false;
    GDIPlusManager();

public:
    ~GDIPlusManager();
    static GDIPlusManager& getInstance();
    bool isAvailable() const;
};

// Classe Base do Componente
class Component {
protected:
    HWND m_hwnd = nullptr;
    int m_x, m_y, m_width, m_height;
    COLORREF m_bgColor = RGB(240, 240, 240);
    COLORREF m_textColor = RGB(0, 0, 0);
    std::wstring m_text;
    HBRUSH m_bgBrush = nullptr;

    std::wstring m_fontFamily = L"Segoe UI";
    float m_fontSize = 12.0f;
    int m_fontStyle = Gdiplus::FontStyleRegular;

    bool m_hasCustomBgColor = false;
    bool m_hasCustomTextColor = false;
    bool m_hasCustomFont = false;

public:
    Component(int x = 0, int y = 0, int width = 0, int height = 0);
    virtual ~Component();

    virtual void updateFont();
    virtual void focus();
    virtual void setPosition(int x, int y);
    virtual void setSize(int width, int height);
    virtual void setBgColor(COLORREF color, bool explicitOverride = true);
    virtual void setTextColor(COLORREF color, bool explicitOverride = true);
    virtual void setFont(const std::wstring& family, float size, int style = Gdiplus::FontStyleRegular, bool explicitOverride = true);
    virtual void setFont(const std::string& family, float size, int style = Gdiplus::FontStyleRegular, bool explicitOverride = true);
    virtual HBRUSH onCtlColor(HDC hdc, HWND hCtl, UINT msg, HBRUSH parentBrush);

    bool hasCustomBgColor() const;
    bool hasCustomTextColor() const;
    bool hasCustomFont() const;

    COLORREF getBgColor() const;
    COLORREF getTextColor() const;
    std::wstring getFontFamily() const;
    float getFontSize() const;
    int getFontStyle() const;
    HBRUSH getBgBrush() const;
    HWND getHWND() const;

    int getX() const;
    int getY() const;
    int getWidth() const;
    int getHeight() const;

    virtual bool handlesHWND(HWND hwnd) const;
    virtual void create(HWND parentHwnd, int id) = 0;
    virtual void draw(LPDRAWITEMSTRUCT dis);
    virtual void triggerClick();
};

class Layout {
public:
    virtual ~Layout() = default;
    virtual void update(Panel* panel) = 0;
};

// Classe do Painel
class Panel : public Component {
protected:
    std::vector<Component*> m_components;
    Layout* m_layout = nullptr;

    static std::vector<Panel*>& getActivePanelsStack();
    void registerClass();
    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    virtual LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam);

public:
    Panel(int x = 0, int y = 0, int width = 0, int height = 0);
    virtual ~Panel();

    virtual bool focusFirstComponent();
    void focus() override;
    void startBuilding();
    void endBuilding();
    static Panel* getActivePanel();

    virtual void adjustToContent();

    void setBgColor(COLORREF color, bool explicitOverride = true) override;
    void setTextColor(COLORREF color, bool explicitOverride = true) override;
    void setFont(const std::wstring& family, float size, int style = Gdiplus::FontStyleRegular, bool explicitOverride = true) override;
    void setFont(const std::string& family, float size, int style = Gdiplus::FontStyleRegular, bool explicitOverride = true) override;

    void setLayout(Layout* layout);
    Layout* getLayout() const;
    void updateLayout();

    const std::vector<Component*>& getComponents() const;
    void addComponent(Component* comp);
    void create(HWND parentHwnd, int id) override;
};

// Classe da Janela Principal
class Window : public Panel {
protected:
    LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam) override;
    void applyDWM();

public:
    Window(const std::wstring& title, int x = 100, int y = 100, int width = 360, int height = 300);
    Window(const std::string& title, int x = 100, int y = 100, int width = 360, int height = 300);

    void setTitle(const std::wstring& title);
    void setTitle(const std::string& title);

    void maximize();
    void minimize();
    void show();
    void runEventLoop();
    void adjustToContent() override;
    void create(HWND parentHwnd, int id) override;
};

// Classe do Botão
class Button : public Component {
public:
    using ClickCallback = void(*)(Component& sender, void* userData);

private:
    ClickCallback m_callback = nullptr;
    void* m_userData = nullptr;

public:
    Button(const std::wstring& label, int x = 0, int y = 0, int width = 0, int height = 35);
    Button(const std::string& label, int x = 0, int y = 0, int width = 0, int height = 35);

    void setLabel(const std::wstring& label);
    void setLabel(const std::string& label);

    void setCallback(ClickCallback cb, void* userData);
    void triggerClick() override;
    void create(HWND parentHwnd, int id) override;
    void draw(LPDRAWITEMSTRUCT dis) override;
};

// Classe de Rótulo (Label Base)
class Label : public Component {
protected:
    Gdiplus::StringAlignment m_align = Gdiplus::StringAlignmentNear;
    bool m_transparent = true;
    HFONT m_hFont = nullptr;

    void updateFont() override;

public:
    Label(const std::wstring& text, int x = 0, int y = 0, int width = 0, int height = 25);
    Label(const std::string& text, int x = 0, int y = 0, int width = 0, int height = 25);
    virtual ~Label();

    void setText(const std::wstring& text);
    void setText(const std::string& text);
    void setAlignment(Gdiplus::StringAlignment align);
    void setTransparent(bool transparent);

    void create(HWND parentHwnd, int id) override;
    void draw(LPDRAWITEMSTRUCT dis) override;
};

class LinkLabel : public Label {
public:
    LinkLabel(const std::wstring& text, int x = 0, int y = 0, int width = 0, int height = 25);
    LinkLabel(const std::string& text, int x = 0, int y = 0, int width = 0, int height = 25);

    void create(HWND parentHwnd, int id) override;
    void draw(LPDRAWITEMSTRUCT dis) override;
};

// Classe do Campo de Entrada
class Input : public Component {
protected:
    std::wstring m_label;
    HWND m_labelHwnd = nullptr;

    COLORREF m_labelTextColor = RGB(0, 0, 0);
    COLORREF m_editBgColor = RGB(255, 255, 255);
    HBRUSH m_editBgBrush = nullptr;

    HFONT m_hFont = nullptr;
    int m_labelHeight = 0;
    int m_editHeight = 0;

    void updateFont() override;

public:
    Input(const std::wstring& label, const std::wstring& initialText = L"", int x = 0, int y = 0, int width = 0, int height = 25);
    Input(const std::string& label, const std::string& initialText = "", int x = 0, int y = 0, int width = 0, int height = 25);
    virtual ~Input();

    bool handlesHWND(HWND hwnd) const override;
    void setBgColor(COLORREF color, bool explicitOverride = true) override;
    void setTextColor(COLORREF color, bool explicitOverride = true) override;
    HBRUSH onCtlColor(HDC hdc, HWND hCtl, UINT msg, HBRUSH parentBrush) override;

    void setLabel(const std::wstring& label);
    void setLabel(const std::string& label);

    void setText(const std::wstring& text);
    void setText(const std::string& text);

    std::wstring getTextW() const;
    std::string getText() const;

    void setPosition(int x, int y) override;
    void setSize(int width, int height) override;
    void create(HWND parentHwnd, int id) override;
};

// Classe de Campo de Saída (Não Editável)
class Output : public Input {
public:
    Output(const std::wstring& label, const std::wstring& initialText = L"", int x = 0, int y = 0, int width = 0, int height = 25);
    Output(const std::string& label, const std::string& initialText = "", int x = 0, int y = 0, int width = 0, int height = 25);

    void create(HWND parentHwnd, int id) override;
    void setReadOnly(bool readOnly);
};

// Layout Vertical
class VerticalLayout : public Layout {
private:
    int m_spacing = 5;
    int m_marginTop = 10;
    int m_marginBottom = 10;
    int m_marginLeft = 10;
    int m_marginRight = 10;
    bool m_autoWidth = true;

public:
    VerticalLayout(int spacing = 5, int margin = 10);
    VerticalLayout(int spacing, int top, int bottom, int left, int right);

    void setSpacing(int spacing);
    int getSpacing() const;

    void setMargin(int margin);
    void setMargins(int top, int bottom, int left, int right);

    void setAutoWidth(bool autoWidth);
    bool isAutoWidth() const;

    void update(Panel* panel) override;
};

// Classe de Tabela
class Table : public Component {
protected:
    HFONT m_hFont = nullptr;
    int m_rows = 0;
    int m_cols = 0;

    int m_cursorRow = 0;
    int m_cursorCol = 0;

    std::vector<std::wstring> m_colHeaders;
    std::vector<std::wstring> m_rowHeaders;
    std::vector<std::vector<std::wstring>> m_data;

    std::wstring indexToLetter(int index) const;
    void syncColumnToView(int colIndex);
    void syncRowToView(int rowIndex);

public:
    Table(int x = 0, int y = 0, int width = 300, int height = 200);
    virtual ~Table() = default;

    // Gerenciamento de Colunas
    void addColumn(const std::wstring& header = L"");
    void addColumn(const std::string& header = "");
    void removeColumn(int index);

    // Gerenciamento de Linhas
    void addRow(const std::wstring& header = L"");
    void addRow(const std::string& header = "");
    void removeRow(int index);

    // Acesso aos Dados
    void setCellText(int row, int col, const std::wstring& text);
    void setCellText(int row, int col, const std::string& text);
    std::wstring getCellTextW(int row, int col) const;
    std::string getCellText(int row, int col) const;

    // Customização de Cabeçalhos
    void setColumnHeader(int col, const std::wstring& text);
    void setColumnHeader(int col, const std::string& text);
    void setRowHeader(int row, const std::wstring& text);
    void setRowHeader(int row, const std::string& text);

    // Consultas de Tamanho
    int getRowCount() const;
    int getColumnCount() const;

    // Seleção Visual
    void selectRow(int row);

    // Navegação via Cursor
    int getCursorRow() const;
    int getCursorCol() const;
    void setCursor(int row, int col);
    void moveCursor(int dRow = 0, int dCol = 1);
    bool isCursorAtEnd() const;

    // Ciclo de vida
    void create(HWND parentHwnd, int id) override;
};

} // namespace oosui

#endif // OOSUI_HPP
