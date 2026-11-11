#include "oosui.hpp"

namespace oosui {

namespace utils {
    std::wstring to_wstring(const std::string& str) {
        if (str.empty()) return L"";
        int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.data(), (int)str.size(), nullptr, 0);
        if (size > 0) {
            std::wstring wstr(size, 0);
            MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.data(), (int)str.size(), &wstr[0], size);
            return wstr;
        }
        size = MultiByteToWideChar(CP_ACP, 0, str.data(), (int)str.size(), nullptr, 0);
        std::wstring wstr(size, 0);
        MultiByteToWideChar(CP_ACP, 0, str.data(), (int)str.size(), &wstr[0], size);
        return wstr;
    }

    std::string to_string(const std::wstring& wstr) {
        if (wstr.empty()) return "";
        int size = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
        std::string str(size, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &str[0], size, nullptr, nullptr);
        return str;
    }
}

void ShowMessage(HWND hwnd, const std::string& msg, const std::string& title) {
    MessageBoxW(hwnd, utils::to_wstring(msg).c_str(), utils::to_wstring(title).c_str(), MB_OK);
}
void ShowMessage(HWND hwnd, const std::wstring& msg, const std::wstring& title) {
    MessageBoxW(hwnd, msg.c_str(), title.c_str(), MB_OK);
}

// --- GDIPlusManager ---
GDIPlusManager::GDIPlusManager() {
    Gdiplus::GdiplusStartupInput input;
    if (Gdiplus::GdiplusStartup(&token, &input, NULL) == Gdiplus::Ok) {
        active = true;
    }
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LINK_CLASS | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);
}

GDIPlusManager::~GDIPlusManager() {
    if (active) Gdiplus::GdiplusShutdown(token);
}

GDIPlusManager& GDIPlusManager::getInstance() {
    static GDIPlusManager instance;
    return instance;
}

bool GDIPlusManager::isAvailable() const { return active; }

// --- Component ---
Component::Component(int x, int y, int width, int height)
    : m_x(x), m_y(y), m_width(width), m_height(height) {
    m_bgBrush = CreateSolidBrush(m_bgColor);
    Panel* activePanel = Panel::getActivePanel();
    if (activePanel && activePanel != this) {
        activePanel->addComponent(this);
    }
}

Component::~Component() {
    if (m_bgBrush) DeleteObject(m_bgBrush);
    if (m_hwnd) DestroyWindow(m_hwnd);
}

void Component::updateFont() {}

void Component::focus() {
    if (m_hwnd) SetFocus(m_hwnd);
}

void Component::setPosition(int x, int y) {
    m_x = x; m_y = y;
    if (m_hwnd) SetWindowPos(m_hwnd, NULL, m_x, m_y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void Component::setSize(int width, int height) {
    m_width = width; m_height = height;
    if (m_hwnd) SetWindowPos(m_hwnd, NULL, 0, 0, m_width, m_height, SWP_NOMOVE | SWP_NOZORDER);
}

void Component::setBgColor(COLORREF color, bool explicitOverride) {
    m_bgColor = color;
    if (explicitOverride) m_hasCustomBgColor = true;
    if (m_bgBrush) DeleteObject(m_bgBrush);
    m_bgBrush = CreateSolidBrush(m_bgColor);
    if (m_hwnd) InvalidateRect(m_hwnd, NULL, TRUE);
}

void Component::setTextColor(COLORREF color, bool explicitOverride) {
    m_textColor = color;
    if (explicitOverride) m_hasCustomTextColor = true;
    if (m_hwnd) InvalidateRect(m_hwnd, NULL, TRUE);
}

void Component::setFont(const std::wstring& family, float size, int style, bool explicitOverride) {
    m_fontFamily = family;
    m_fontSize = size;
    m_fontStyle = style;
    if (explicitOverride) m_hasCustomFont = true;
    updateFont();
    if (m_hwnd) InvalidateRect(m_hwnd, NULL, TRUE);
}

void Component::setFont(const std::string& family, float size, int style, bool explicitOverride) {
    setFont(utils::to_wstring(family), size, style, explicitOverride);
}

HBRUSH Component::onCtlColor(HDC hdc, HWND hCtl, UINT msg, HBRUSH parentBrush) {
    SetTextColor(hdc, m_textColor);
    SetBkColor(hdc, m_bgColor);
    return m_bgBrush;
}

bool Component::hasCustomBgColor() const { return m_hasCustomBgColor; }
bool Component::hasCustomTextColor() const { return m_hasCustomTextColor; }
bool Component::hasCustomFont() const { return m_hasCustomFont; }

COLORREF Component::getBgColor() const { return m_bgColor; }
COLORREF Component::getTextColor() const { return m_textColor; }
std::wstring Component::getFontFamily() const { return m_fontFamily; }
float Component::getFontSize() const { return m_fontSize; }
int Component::getFontStyle() const { return m_fontStyle; }
HBRUSH Component::getBgBrush() const { return m_bgBrush; }
HWND Component::getHWND() const { return m_hwnd; }

int Component::getX() const { return m_x; }
int Component::getY() const { return m_y; }
int Component::getWidth() const { return m_width; }
int Component::getHeight() const { return m_height; }

bool Component::handlesHWND(HWND hwnd) const { return m_hwnd == hwnd; }
void Component::draw(LPDRAWITEMSTRUCT dis) {}
void Component::triggerClick() {}

// --- Panel ---
std::vector<Panel*>& Panel::getActivePanelsStack() {
    static std::vector<Panel*> panels;
    return panels;
}

void Panel::registerClass() {
    static bool registered = false;
    if (registered) return;

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = Panel::StaticWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"OOSUI_PanelClass";
    RegisterClassExW(&wc);
    registered = true;
}

LRESULT CALLBACK Panel::StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Panel* pThis = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pThis = reinterpret_cast<Panel*>(pCreate->lpCreateParams);
        if (pThis) {
            pThis->m_hwnd = hwnd;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        }
    } else {
        pThis = reinterpret_cast<Panel*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    return pThis ? pThis->WndProc(msg, wParam, lParam) : DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT Panel::WndProc(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_SETFOCUS: {
        HWND hFirst = GetNextDlgTabItem(m_hwnd, NULL, FALSE);
        if (hFirst && hFirst != m_hwnd) {
            SetFocus(hFirst);
            return 0;
        }
        break;
    }
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rect;
        GetClientRect(m_hwnd, &rect);
        FillRect(hdc, &rect, m_bgBrush);
        return 1;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT: {
        HDC hdcStatic = (HDC)wParam;
        HWND hCtl = (HWND)lParam;
        for (auto* comp : m_components) {
            if (comp->handlesHWND(hCtl)) {
                return (INT_PTR)comp->onCtlColor(hdcStatic, hCtl, msg, m_bgBrush);
            }
        }
        SetBkMode(hdcStatic, TRANSPARENT);
        return (INT_PTR)m_bgBrush;
    }
    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
        for (auto* comp : m_components) {
            if (comp->getHWND() == dis->hwndItem) {
                comp->draw(dis);
                return TRUE;
            }
        }
        break;
    }
    case WM_COMMAND: {
        HWND hCtl = (HWND)lParam;
        if (HIWORD(wParam) == BN_CLICKED && hCtl) {
            for (auto* comp : m_components) {
                if (comp->getHWND() == hCtl) {
                    comp->triggerClick();
                    break;
                }
            }
        }
        break;
    }
    case WM_NOTIFY: {
        LPNMHDR pnmh = (LPNMHDR)lParam;
        if (pnmh->code == NM_CLICK || pnmh->code == NM_RETURN) {
            PNMLINK pNMLink = (PNMLINK)lParam;
            for (auto* comp : m_components) {
                if (comp->getHWND() == pnmh->hwndFrom) {
                    ShellExecuteW(NULL, L"open", pNMLink->item.szUrl, NULL, NULL, SW_SHOWNORMAL);
                    return 1;
                }
            }
        }
        break;
    }
    case WM_SIZE: {
        m_width = LOWORD(lParam);
        m_height = HIWORD(lParam);
        updateLayout();
        break;
    }
    }
    return DefWindowProc(m_hwnd, msg, wParam, lParam);
}

Panel::Panel(int x, int y, int width, int height) : Component(x, y, width, height) {
    m_layout = new VerticalLayout();
    startBuilding();
}

Panel::~Panel() {
    if (m_layout) {
        delete m_layout;
    }
    endBuilding();
}

bool Panel::focusFirstComponent() {
    if (!m_hwnd) return false;
    HWND hFirst = GetNextDlgTabItem(m_hwnd, NULL, FALSE);
    if (hFirst && hFirst != m_hwnd) {
        SetFocus(hFirst);
        return true;
    }
    return false;
}

void Panel::focus() {
    if (!focusFirstComponent()) {
        Component::focus();
    }
}

void Panel::startBuilding() { getActivePanelsStack().push_back(this); }

void Panel::endBuilding() {
    auto& panels = getActivePanelsStack();
    for (size_t i = panels.size(); i > 0; --i) {
        if (panels[i - 1] == this) {
            panels.erase(panels.begin() + i - 1);
            break;
        }
    }
}

Panel* Panel::getActivePanel() {
    auto& panels = getActivePanelsStack();
    return panels.empty() ? nullptr : panels.back();
}

void Panel::setBgColor(COLORREF color, bool explicitOverride) {
    Component::setBgColor(color, explicitOverride);
    for (auto* comp : m_components) {
        if (!comp->hasCustomBgColor()) comp->setBgColor(color, false);
    }
}

void Panel::setTextColor(COLORREF color, bool explicitOverride) {
    Component::setTextColor(color, explicitOverride);
    for (auto* comp : m_components) {
        if (!comp->hasCustomTextColor()) comp->setTextColor(color, false);
    }
}

void Panel::setFont(const std::wstring& family, float size, int style, bool explicitOverride) {
    Component::setFont(family, size, style, explicitOverride);
    for (auto* comp : m_components) {
        if (!comp->hasCustomFont()) {
            comp->setFont(family, size, style, false);
        }
    }
}

void Panel::setFont(const std::string& family, float size, int style, bool explicitOverride) {
    setFont(utils::to_wstring(family), size, style, explicitOverride);
}

void Panel::setLayout(Layout* layout) {
    if (m_layout && m_layout != layout) {
        delete m_layout;
    }
    m_layout = layout;
    updateLayout();
}

Layout* Panel::getLayout() const { return m_layout; }

void Panel::updateLayout() {
    if (m_layout) {
        m_layout->update(this);
    }
}

const std::vector<Component*>& Panel::getComponents() const { return m_components; }

void Panel::addComponent(Component* comp) {
    if (comp && std::find(m_components.begin(), m_components.end(), comp) == m_components.end()) {
        m_components.push_back(comp);

        if (!comp->hasCustomBgColor()) comp->setBgColor(m_bgColor, false);
        if (!comp->hasCustomTextColor()) comp->setTextColor(m_textColor, false);
        if (!comp->hasCustomFont()) comp->setFont(m_fontFamily, m_fontSize, m_fontStyle, false);

        if (m_hwnd) comp->create(m_hwnd, static_cast<int>(m_components.size()));
        updateLayout();
    }
}

void Panel::adjustToContent() {
    if (!m_hwnd) return;

    int currentY = 10; // Margem superior padrão
    int spacing = 5;   // Espaçamento padrão
    int marginBottom = 10;

    if (m_layout) {
        VerticalLayout* vLayout = dynamic_cast<VerticalLayout*>(m_layout);
        if (vLayout) {
            spacing = vLayout->getSpacing();
        }
    }

    for (auto* comp : m_components) {
        if (comp) {
            currentY += comp->getHeight() + spacing;
        }
    }

    int neededHeight = (currentY > 10) ? (currentY - spacing + marginBottom) : m_height;

    // Para Painéis normais (sem moldura de janela), ajusta diretamente o tamanho útil
    SetWindowPos(m_hwnd, NULL, 0, 0, m_width, neededHeight, SWP_NOMOVE | SWP_NOZORDER);
}

void Panel::create(HWND parentHwnd, int id) {
    registerClass();
    m_hwnd = CreateWindowExW(
        WS_EX_CONTROLPARENT, L"OOSUI_PanelClass", L"", WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
        m_x, m_y, m_width, m_height, parentHwnd, (HMENU)(UINT_PTR)id, GetModuleHandle(NULL), this
    );
    int childId = 1;
    for (auto* comp : m_components) comp->create(m_hwnd, childId++);

    // Executa o ajuste de conteúdo do painel apenas na criação
    adjustToContent();
}

// --- Window ---
LRESULT Window::WndProc(UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return Panel::WndProc(msg, wParam, lParam);
}

void Window::applyDWM() { DwmEnableBlurBehindWindow(m_hwnd, NULL); }

Window::Window(const std::wstring& title, int x, int y, int width, int height)
    : Panel(x, y, width, height) {
    m_text = title;
    GDIPlusManager::getInstance();
}

Window::Window(const std::string& title, int x, int y, int width, int height)
    : Window(utils::to_wstring(title), x, y, width, height) {}

void Window::setTitle(const std::wstring& title) {
    m_text = title;
    if (m_hwnd) SetWindowTextW(m_hwnd, m_text.c_str());
}

void Window::setTitle(const std::string& title) { setTitle(utils::to_wstring(title)); }

void Window::maximize() { if (m_hwnd) ShowWindow(m_hwnd, SW_MAXIMIZE); }
void Window::minimize() { if (m_hwnd) ShowWindow(m_hwnd, SW_MINIMIZE); }

void Window::show() {
    endBuilding();
    if (!m_hwnd) create(NULL, 0);
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);
    focusFirstComponent();
}

void Window::runEventLoop() {
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!IsDialogMessage(m_hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

void Window::adjustToContent() {
    if (!m_hwnd) return;

    int currentY = 10;
    int spacing = 5;
    int marginBottom = 10;

    if (m_layout) {
        VerticalLayout* vLayout = dynamic_cast<VerticalLayout*>(m_layout);
        if (vLayout) {
            spacing = vLayout->getSpacing();
        }
    }

    for (auto* comp : m_components) {
        if (comp) {
            currentY += comp->getHeight() + spacing;
        }
    }

    int neededClientHeight = (currentY > 10) ? (currentY - spacing + marginBottom) : m_height;

    // Para a Janela Principal, calcula a área externa somando bordas e barra de título
    LONG style = GetWindowLongW(m_hwnd, GWL_STYLE);
    LONG exStyle = GetWindowLongW(m_hwnd, GWL_EXSTYLE);

    RECT rect = { 0, 0, m_width, neededClientHeight };
    AdjustWindowRectEx(&rect, style, FALSE, exStyle);

    int newWidth = rect.right - rect.left;
    int newHeight = rect.bottom - rect.top;

    SetWindowPos(m_hwnd, NULL, 0, 0, newWidth, newHeight, SWP_NOMOVE | SWP_NOZORDER);
}

void Window::create(HWND parentHwnd, int id) {
    registerClass();
    m_hwnd = CreateWindowExW(
        WS_EX_CONTROLPARENT, L"OOSUI_PanelClass", m_text.c_str(), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        m_x, m_y, m_width, m_height, NULL, NULL, GetModuleHandle(NULL), this
    );
    applyDWM();
    int childId = 1;
    for (auto* comp : m_components) comp->create(m_hwnd, childId++);

    // Ajusta a altura da janela ao conteúdo apenas uma vez durante a criação
    adjustToContent();
}

// --- Button ---
Button::Button(const std::wstring& label, int x, int y, int width, int height)
    : Component(x, y, width, height) {
    m_text = label;
    m_bgColor = RGB(225, 225, 225);
}

Button::Button(const std::string& label, int x, int y, int width, int height)
    : Button(utils::to_wstring(label), x, y, width, height) {}

void Button::setLabel(const std::wstring& label) {
    m_text = label;
    if (m_hwnd) SetWindowTextW(m_hwnd, m_text.c_str());
}

void Button::setLabel(const std::string& label) { setLabel(utils::to_wstring(label)); }

void Button::setCallback(ClickCallback cb, void* userData) { m_callback = cb; m_userData = userData; }

void Button::triggerClick() { if (m_callback) m_callback(*this, m_userData); }

void Button::create(HWND parentHwnd, int id) {
    m_hwnd = CreateWindowExW(
        0, L"BUTTON", m_text.c_str(), WS_VISIBLE | WS_CHILD | BS_OWNERDRAW | WS_TABSTOP,
        m_x, m_y, m_width, m_height, parentHwnd, (HMENU)(UINT_PTR)id, GetModuleHandle(NULL), NULL
    );
}

void Button::draw(LPDRAWITEMSTRUCT dis) {
    HDC hdc = dis->hDC;
    RECT rect = dis->rcItem;
    bool isPressed = (dis->itemState & ODS_SELECTED);
    bool isFocused = (dis->itemState & ODS_FOCUS);

    if (GDIPlusManager::getInstance().isAvailable()) {
        Gdiplus::Graphics g(hdc);
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

        COLORREF fill = isPressed ? RGB(180, 180, 180) : m_bgColor;
        Gdiplus::SolidBrush bgBrush(Gdiplus::Color(255, GetRValue(fill), GetGValue(fill), GetBValue(fill)));
        g.FillRectangle(&bgBrush, (INT)rect.left, (INT)rect.top, (INT)(rect.right - rect.left), (INT)(rect.bottom - rect.top));

        COLORREF borderColor = isFocused ? RGB(0, 120, 215) : RGB(140, 140, 140);
        float penWidth = isFocused ? 2.0f : 1.0f;
        Gdiplus::Pen borderPen(Gdiplus::Color(255, GetRValue(borderColor), GetGValue(borderColor), GetBValue(borderColor)), penWidth);

        int pad = isFocused ? 1 : 0;
        g.DrawRectangle(&borderPen, (INT)rect.left + pad, (INT)rect.top + pad, (INT)(rect.right - rect.left - 1 - (pad*2)), (INT)(rect.bottom - rect.top - 1 - (pad*2)));

        Gdiplus::FontFamily fontFamily(m_fontFamily.c_str());
        Gdiplus::Font font(&fontFamily, m_fontSize, m_fontStyle, Gdiplus::UnitPoint);
        Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, GetRValue(m_textColor), GetGValue(m_textColor), GetBValue(m_textColor)));

        Gdiplus::StringFormat format;
        format.SetAlignment(Gdiplus::StringAlignmentCenter);
        format.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        format.SetHotkeyPrefix(Gdiplus::HotkeyPrefixShow);

        Gdiplus::RectF rectF((float)rect.left, (float)rect.top, (float)(rect.right - rect.left), (float)(rect.bottom - rect.top));
        g.DrawString(m_text.c_str(), -1, &font, rectF, &format, &textBrush);
    }
}

// --- Label ---
void Label::updateFont() {
    if (m_hFont) DeleteObject(m_hFont);

    HDC hdc = GetDC(NULL);
    int height = -MulDiv((int)m_fontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(NULL, hdc);

    int weight = (m_fontStyle & Gdiplus::FontStyleBold) ? FW_BOLD : FW_NORMAL;
    DWORD italic = (m_fontStyle & Gdiplus::FontStyleItalic) ? TRUE : FALSE;

    m_hFont = CreateFontW(
        height, 0, 0, 0, weight, italic, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, m_fontFamily.c_str()
    );

    if (m_hwnd) {
        SendMessageW(m_hwnd, WM_SETFONT, (WPARAM)m_hFont, TRUE);
        InvalidateRect(m_hwnd, NULL, TRUE);
    }
}

Label::Label(const std::wstring& text, int x, int y, int width, int height)
    : Component(x, y, width, height) {
    m_text = text;
}

Label::Label(const std::string& text, int x, int y, int width, int height)
    : Label(utils::to_wstring(text), x, y, width, height) {}

Label::~Label() {
    if (m_hFont) DeleteObject(m_hFont);
}

void Label::setText(const std::wstring& text) {
    m_text = text;
    if (m_hwnd) {
        SetWindowTextW(m_hwnd, m_text.c_str());
        InvalidateRect(m_hwnd, NULL, TRUE);
    }
}

void Label::setText(const std::string& text) { setText(utils::to_wstring(text)); }

void Label::setAlignment(Gdiplus::StringAlignment align) {
    m_align = align;
    if (m_hwnd) InvalidateRect(m_hwnd, NULL, TRUE);
}

void Label::setTransparent(bool transparent) {
    m_transparent = transparent;
    if (m_hwnd) InvalidateRect(m_hwnd, NULL, TRUE);
}

void Label::create(HWND parentHwnd, int id) {
    m_hwnd = CreateWindowExW(
        0, L"STATIC", m_text.c_str(),
        WS_VISIBLE | WS_CHILD | SS_OWNERDRAW | SS_NOTIFY,
        m_x, m_y, m_width, m_height,
        parentHwnd, (HMENU)(UINT_PTR)id,
        GetModuleHandle(NULL), NULL
    );
    if (m_hFont) SendMessageW(m_hwnd, WM_SETFONT, (WPARAM)m_hFont, TRUE);
}

void Label::draw(LPDRAWITEMSTRUCT dis) {
    HDC hdc = dis->hDC;
    RECT rect = dis->rcItem;

    if (GDIPlusManager::getInstance().isAvailable()) {
        if (m_transparent) {
            DrawThemeParentBackground(dis->hwndItem, hdc, &rect);
        } else {
            Gdiplus::Graphics g_bg(hdc);
            Gdiplus::SolidBrush bgBrush(Gdiplus::Color(255, GetRValue(m_bgColor), GetGValue(m_bgColor), GetBValue(m_bgColor)));
            g_bg.FillRectangle(&bgBrush, (INT)rect.left, (INT)rect.top, (INT)(rect.right - rect.left), (INT)(rect.bottom - rect.top));
        }

        Gdiplus::Graphics g(hdc);
        g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

        Gdiplus::FontFamily fontFamily(m_fontFamily.c_str());
        Gdiplus::Font font(&fontFamily, m_fontSize, m_fontStyle, Gdiplus::UnitPoint);
        Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, GetRValue(m_textColor), GetGValue(m_textColor), GetBValue(m_textColor)));

        Gdiplus::StringFormat format;
        format.SetAlignment(m_align);
        format.SetLineAlignment(Gdiplus::StringAlignmentNear);
        format.SetHotkeyPrefix(Gdiplus::HotkeyPrefixShow);

        Gdiplus::RectF rectF((float)rect.left, (float)rect.top, (float)(rect.right - rect.left), (float)(rect.bottom - rect.top));
        g.DrawString(m_text.c_str(), -1, &font, rectF, &format, &textBrush);
    }
}

// --- LinkLabel ---
LinkLabel::LinkLabel(const std::wstring& text, int x, int y, int width, int height)
    : Label(text, x, y, width, height) {}

LinkLabel::LinkLabel(const std::string& text, int x, int y, int width, int height)
    : Label(text, x, y, width, height) {}

void LinkLabel::create(HWND parentHwnd, int id) {
    m_hwnd = CreateWindowExW(
        0, L"SysLink", m_text.c_str(),
        WS_VISIBLE | WS_CHILD | WS_TABSTOP | LWS_TRANSPARENT,
        m_x, m_y, m_width, m_height,
        parentHwnd, (HMENU)(UINT_PTR)id,
        GetModuleHandle(NULL), NULL
    );

    if (m_hFont) SendMessageW(m_hwnd, WM_SETFONT, (WPARAM)m_hFont, TRUE);
}

void LinkLabel::draw(LPDRAWITEMSTRUCT dis) {}

// --- Input ---
void Input::updateFont() {
    if (m_hFont) DeleteObject(m_hFont);

    HDC hdc = GetDC(NULL);
    int heightPx = MulDiv((int)m_fontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(NULL, hdc);

    int weight = (m_fontStyle & Gdiplus::FontStyleBold) ? FW_BOLD : FW_NORMAL;
    DWORD italic = (m_fontStyle & Gdiplus::FontStyleItalic) ? TRUE : FALSE;

    m_hFont = CreateFontW(
        -heightPx, 0, 0, 0, weight, italic, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, m_fontFamily.c_str()
    );

    m_labelHeight = m_label.empty() ? 0 : heightPx + 4;
    m_height = m_editHeight + m_labelHeight;

    if (m_labelHwnd) SendMessageW(m_labelHwnd, WM_SETFONT, (WPARAM)m_hFont, TRUE);
    if (m_hwnd) SendMessageW(m_hwnd, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    if (m_hwnd || m_labelHwnd) setSize(m_width, m_height);
}

Input::Input(const std::wstring& label, const std::wstring& initialText, int x, int y, int width, int height)
    : Component(x, y, width, height), m_label(label), m_editHeight(height) {
    m_text = initialText;
    updateFont();
}

Input::Input(const std::string& label, const std::string& initialText, int x, int y, int width, int height)
    : Input(utils::to_wstring(label), utils::to_wstring(initialText), x, y, width, height) {}

Input::~Input() {
    if (m_hFont) DeleteObject(m_hFont);
    if (m_editBgBrush) DeleteObject(m_editBgBrush);
}

bool Input::handlesHWND(HWND hwnd) const {
    return m_hwnd == hwnd || m_labelHwnd == hwnd;
}

void Input::setBgColor(COLORREF color, bool explicitOverride) {
    if (explicitOverride) {
        m_editBgColor = color;
        m_hasCustomBgColor = true;
        if (m_editBgBrush) DeleteObject(m_editBgBrush);
        m_editBgBrush = CreateSolidBrush(m_editBgColor);
    }
    if (m_labelHwnd) InvalidateRect(m_labelHwnd, NULL, TRUE);
    if (m_hwnd) InvalidateRect(m_hwnd, NULL, TRUE);
}

void Input::setTextColor(COLORREF color, bool explicitOverride) {
    if (explicitOverride) {
        m_textColor = color;
        m_hasCustomTextColor = true;
    }
    m_labelTextColor = color;
    if (m_labelHwnd) InvalidateRect(m_labelHwnd, NULL, TRUE);
    if (m_hwnd) InvalidateRect(m_hwnd, NULL, TRUE);
}

HBRUSH Input::onCtlColor(HDC hdc, HWND hCtl, UINT msg, HBRUSH parentBrush) {
    if (hCtl == m_labelHwnd) {
        SetTextColor(hdc, m_labelTextColor);
        SetBkMode(hdc, TRANSPARENT);
        return parentBrush;
    }
    if (hCtl == m_hwnd) {
        SetTextColor(hdc, m_textColor);
        SetBkColor(hdc, m_editBgColor);
        if (!m_editBgBrush) m_editBgBrush = CreateSolidBrush(m_editBgColor);
        return m_editBgBrush;
    }
    return Component::onCtlColor(hdc, hCtl, msg, parentBrush);
}

void Input::setLabel(const std::wstring& label) {
    m_label = label;
    if (m_labelHwnd) SetWindowTextW(m_labelHwnd, m_label.c_str());
}

void Input::setLabel(const std::string& label) { setLabel(utils::to_wstring(label)); }

void Input::setText(const std::wstring& text) {
    m_text = text;
    if (m_hwnd) SetWindowTextW(m_hwnd, m_text.c_str());
}

void Input::setText(const std::string& text) { setText(utils::to_wstring(text)); }

std::wstring Input::getTextW() const {
    if (m_hwnd) {
        int len = GetWindowTextLengthW(m_hwnd);
        if (len > 0) {
            std::wstring buf(len + 1, L'\0');
            GetWindowTextW(m_hwnd, &buf[0], len + 1);
            buf.resize(len);
            return buf;
        }
        return L"";
    }
    return m_text;
}

std::string Input::getText() const { return utils::to_string(getTextW()); }

void Input::setPosition(int x, int y) {
    m_x = x; m_y = y;
    int currentY = m_y;

    if (m_labelHwnd) {
        SetWindowPos(m_labelHwnd, NULL, m_x, currentY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        currentY += m_labelHeight;
    }
    if (m_hwnd) {
        SetWindowPos(m_hwnd, NULL, m_x, currentY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
}

void Input::setSize(int width, int height) {
    m_width = width;
    m_height = height;

    if (m_labelHwnd) {
        SetWindowPos(m_labelHwnd, NULL, 0, 0, m_width, m_labelHeight, SWP_NOMOVE | SWP_NOZORDER);
    }
    if (m_hwnd) {
        int editBoxHeight = m_height - m_labelHeight;
        SetWindowPos(m_hwnd, NULL, 0, 0, m_width, editBoxHeight, SWP_NOMOVE | SWP_NOZORDER);
    }
}

void Input::create(HWND parentHwnd, int id) {
    int currentY = m_y;
    if (!m_label.empty()) {
        m_labelHwnd = CreateWindowExW(
            0, L"STATIC", m_label.c_str(), WS_VISIBLE | WS_CHILD | SS_LEFT,
            m_x, m_y, m_width, m_labelHeight, parentHwnd, NULL, GetModuleHandle(NULL), NULL
        );
        currentY += m_labelHeight;
    }

    int editBoxHeight = m_height - m_labelHeight;

    m_hwnd = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", m_text.c_str(), WS_VISIBLE | WS_CHILD | ES_LEFT | ES_AUTOHSCROLL | WS_TABSTOP,
        m_x, currentY, m_width, editBoxHeight, parentHwnd, (HMENU)(UINT_PTR)id, GetModuleHandle(NULL), NULL
    );

    if (m_hFont) {
        if (m_labelHwnd) SendMessageW(m_labelHwnd, WM_SETFONT, (WPARAM)m_hFont, TRUE);
        if (m_hwnd) SendMessageW(m_hwnd, WM_SETFONT, (WPARAM)m_hFont, TRUE);
    }
}

// --- Output ---
Output::Output(const std::wstring& label, const std::wstring& initialText, int x, int y, int width, int height)
    : Input(label, initialText, x, y, width, height) {}

Output::Output(const std::string& label, const std::string& initialText, int x, int y, int width, int height)
    : Output(utils::to_wstring(label), utils::to_wstring(initialText), x, y, width, height) {}

void Output::create(HWND parentHwnd, int id) {
    Input::create(parentHwnd, id);
    if (m_hwnd) {
        SendMessageW(m_hwnd, EM_SETREADONLY, TRUE, 0);
    }
}

void Output::setReadOnly(bool readOnly) {
    if (m_hwnd) {
        SendMessageW(m_hwnd, EM_SETREADONLY, readOnly ? TRUE : FALSE, 0);
    }
}

// --- VerticalLayout ---
VerticalLayout::VerticalLayout(int spacing, int margin)
    : m_spacing(spacing), m_marginTop(margin), m_marginBottom(margin),
      m_marginLeft(margin), m_marginRight(margin) {}

VerticalLayout::VerticalLayout(int spacing, int top, int bottom, int left, int right)
    : m_spacing(spacing), m_marginTop(top), m_marginBottom(bottom),
      m_marginLeft(left), m_marginRight(right) {}

void VerticalLayout::setSpacing(int spacing) { m_spacing = spacing; }
int VerticalLayout::getSpacing() const { return m_spacing; }

void VerticalLayout::setMargin(int margin) {
    m_marginTop = m_marginBottom = m_marginLeft = m_marginRight = margin;
}

void VerticalLayout::setMargins(int top, int bottom, int left, int right) {
    m_marginTop = top;
    m_marginBottom = bottom;
    m_marginLeft = left;
    m_marginRight = right;
}

void VerticalLayout::setAutoWidth(bool autoWidth) { m_autoWidth = autoWidth; }
bool VerticalLayout::isAutoWidth() const { return m_autoWidth; }

void VerticalLayout::update(Panel* panel) {
    if (!panel) return;

    const auto& components = panel->getComponents();
    int currentY = m_marginTop;
    int availableWidth = panel->getWidth() - m_marginLeft - m_marginRight;

    for (auto* comp : components) {
        if (!comp) continue;

        int compX = m_marginLeft;
        int compY = currentY;
        int compWidth = m_autoWidth && availableWidth > 0 ? availableWidth : comp->getWidth();
        int compHeight = comp->getHeight();

        comp->setPosition(compX, compY);
        comp->setSize(compWidth, compHeight);

        currentY += compHeight + m_spacing;
    }
}

// --- Table ---
Table::Table(int x, int y, int width, int height)
    : Component(x, y, width, height) {}

std::wstring Table::indexToLetter(int index) const {
    std::wstring letter;
    while (index >= 0) {
        letter = (wchar_t)(L'A' + (index % 26)) + letter;
        index = (index / 26) - 1;
    }
    return letter;
}

void Table::addColumn(const std::wstring& header) {
    std::wstring headTxt = header.empty() ? indexToLetter(m_cols) : header;
    m_colHeaders.push_back(headTxt);

    for (auto& row : m_data) {
        row.push_back(L"");
    }
    m_cols++;

    if (m_hwnd) {
        syncColumnToView(m_cols - 1);
    }
}

void Table::addColumn(const std::string& header) { addColumn(utils::to_wstring(header)); }

void Table::removeColumn(int index) {
    if (index < 0 || index >= m_cols) return;

    m_colHeaders.erase(m_colHeaders.begin() + index);
    for (auto& row : m_data) {
        row.erase(row.begin() + index);
    }
    m_cols--;

    if (m_hwnd) {
        ListView_DeleteColumn(m_hwnd, index + 1); // +1 devido à coluna de cabeçalho de linha
    }
}

void Table::addRow(const std::wstring& header) {
    std::wstring headTxt = header.empty() ? std::to_wstring(m_rows + 1) : header;
    m_rowHeaders.push_back(headTxt);
    m_data.push_back(std::vector<std::wstring>(m_cols, L""));
    m_rows++;

    if (m_hwnd) {
        syncRowToView(m_rows - 1);
    }
}

void Table::addRow(const std::string& header) { addRow(utils::to_wstring(header)); }

void Table::removeRow(int index) {
    if (index < 0 || index >= m_rows) return;

    m_rowHeaders.erase(m_rowHeaders.begin() + index);
    m_data.erase(m_data.begin() + index);
    m_rows--;

    if (m_hwnd) {
        ListView_DeleteItem(m_hwnd, index);
    }
}

void Table::setCellText(int row, int col, const std::wstring& text) {
    if (row < 0 || row >= m_rows || col < 0 || col >= m_cols) return;
    m_data[row][col] = text;

    if (m_hwnd) {
        ListView_SetItemText(m_hwnd, row, col + 1, (LPWSTR)m_data[row][col].c_str());
    }
}

void Table::setCellText(int row, int col, const std::string& text) {
    setCellText(row, col, utils::to_wstring(text));
}

std::wstring Table::getCellTextW(int row, int col) const {
    if (row < 0 || row >= m_rows || col < 0 || col >= m_cols) return L"";
    return m_data[row][col];
}

std::string Table::getCellText(int row, int col) const {
    return utils::to_string(getCellTextW(row, col));
}

void Table::setColumnHeader(int col, const std::wstring& text) {
    if (col < 0 || col >= m_cols) return;
    m_colHeaders[col] = text;
    if (m_hwnd) {
        LVCOLUMNW lvc = {0};
        lvc.mask = LVCF_TEXT;
        lvc.pszText = (LPWSTR)m_colHeaders[col].c_str();
        SendMessageW(m_hwnd, LVM_SETCOLUMNW, col + 1, (LPARAM)&lvc);
    }
}

void Table::setColumnHeader(int col, const std::string& text) { setColumnHeader(col, utils::to_wstring(text)); }

void Table::setRowHeader(int row, const std::wstring& text) {
    if (row < 0 || row >= m_rows) return;
    m_rowHeaders[row] = text;
    if (m_hwnd) {
        ListView_SetItemText(m_hwnd, row, 0, (LPWSTR)m_rowHeaders[row].c_str());
    }
}

void Table::setRowHeader(int row, const std::string& text) { setRowHeader(row, utils::to_wstring(text)); }

int Table::getRowCount() const { return m_rows; }
int Table::getColumnCount() const { return m_cols; }

void Table::selectRow(int row) {
    if (m_hwnd && row >= 0 && row < m_rows) {
        ListView_SetItemState(m_hwnd, -1, 0, LVIS_SELECTED); // Limpa seleção atual
        ListView_SetItemState(m_hwnd, row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(m_hwnd, row, FALSE);
    }
}

int Table::getCursorRow() const { return m_cursorRow; }
int Table::getCursorCol() const { return m_cursorCol; }

void Table::setCursor(int row, int col) {
    m_cursorRow = row;
    m_cursorCol = col;
}

void Table::moveCursor(int dRow, int dCol) {
    m_cursorCol += dCol;
    m_cursorRow += dRow;

    // Deslocamento quebra para a próxima linha se ultrapassar as colunas
    while (m_cursorCol >= m_cols && m_cols > 0) {
        m_cursorCol -= m_cols;
        m_cursorRow++;
    }
    while (m_cursorCol < 0 && m_cols > 0) {
        m_cursorCol += m_cols;
        m_cursorRow--;
    }

    if (m_cursorRow < 0) m_cursorRow = 0;
}

bool Table::isCursorAtEnd() const {
    return m_cursorRow >= m_rows;
}

void Table::syncColumnToView(int colIndex) {
    LVCOLUMNW lvc = {0};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    lvc.cx = 100;
    lvc.pszText = (LPWSTR)m_colHeaders[colIndex].c_str();
    lvc.iSubItem = colIndex + 1;
    SendMessageW(m_hwnd, LVM_INSERTCOLUMNW, colIndex + 1, (LPARAM)&lvc);

    // Preenche com os dados existentes
    for (int r = 0; r < m_rows; ++r) {
        ListView_SetItemText(m_hwnd, r, colIndex + 1, (LPWSTR)m_data[r][colIndex].c_str());
    }
}

void Table::syncRowToView(int rowIndex) {
    LVITEMW lvi = {0};
    lvi.mask = LVIF_TEXT;
    lvi.iItem = rowIndex;
    lvi.iSubItem = 0;
    lvi.pszText = (LPWSTR)m_rowHeaders[rowIndex].c_str();
    SendMessageW(m_hwnd, LVM_INSERTITEMW, 0, (LPARAM)&lvi);

    // Preenche colunas da linha
    for (int c = 0; c < m_cols; ++c) {
        ListView_SetItemText(m_hwnd, rowIndex, c + 1, (LPWSTR)m_data[rowIndex][c].c_str());
    }
}

void Table::create(HWND parentHwnd, int id) {
    m_hwnd = CreateWindowExW(
        WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | WS_TABSTOP | WS_BORDER,
        m_x, m_y, m_width, m_height, parentHwnd, (HMENU)(UINT_PTR)id, GetModuleHandle(NULL), NULL
    );

    // Aplica estilos de grade e seleção de linha inteira
    ListView_SetExtendedListViewStyle(m_hwnd, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

    if (m_hFont) SendMessageW(m_hwnd, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    // Coluna 0 dedicada para o Cabeçalho da Linha (ex: 1, 2, 3)
    LVCOLUMNW lvcHeader = {0};
    lvcHeader.mask = LVCF_TEXT | LVCF_WIDTH;
    lvcHeader.cx = 40; // Largura do cabeçalho da linha
    lvcHeader.pszText = (LPWSTR)L"";
    SendMessageW(m_hwnd, LVM_INSERTCOLUMNW, 0, (LPARAM)&lvcHeader);

    // Sincroniza estado lógico do C++ com a API Win32
    for (int c = 0; c < m_cols; ++c) {
        syncColumnToView(c);
    }
    for (int r = 0; r < m_rows; ++r) {
        syncRowToView(r);
    }
}

} // namespace oosui
