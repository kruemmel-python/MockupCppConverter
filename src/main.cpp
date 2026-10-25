#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct Node { std::wstring tag, text, classes, id, action; int depth = 0; };
struct Analysis { std::wstring title; std::vector<Node> nodes; int buttons=0, inputs=0, headings=0; };

static HWND g_file, g_output, g_status, g_stats;
static std::wstring W(const std::string& value) {
    if (value.empty()) return L"";
    int n=MultiByteToWideChar(CP_UTF8,0,value.data(),(int)value.size(),nullptr,0);
    std::wstring out(n,L' '); MultiByteToWideChar(CP_UTF8,0,value.data(),(int)value.size(),out.data(),n); return out;
}
static std::string U(const std::wstring& value) {
    if (value.empty()) return "";
    int n=WideCharToMultiByte(CP_UTF8,0,value.data(),(int)value.size(),nullptr,0,nullptr,nullptr);
    std::string out(n,' '); WideCharToMultiByte(CP_UTF8,0,value.data(),(int)value.size(),out.data(),n,nullptr,nullptr); return out;
}
static std::wstring Trim(std::wstring s) { auto a=s.find_first_not_of(L" \t\r\n"); auto b=s.find_last_not_of(L" \t\r\n"); return a==std::wstring::npos?L"":s.substr(a,b-a+1); }
static std::wstring HtmlText(std::wstring s) {
    s=std::regex_replace(s,std::wregex(LR"(<[^>]*>)"),L"");
    s=std::regex_replace(s,std::wregex(LR"(&nbsp;)"),L" "); s=std::regex_replace(s,std::wregex(LR"(&amp;)"),L"&");
    s=std::regex_replace(s,std::wregex(LR"(&[a-zA-Z]+;)"),L""); return Trim(s);
}
static std::wstring Attr(const std::wstring& tag, const std::wstring& key) {
    std::wregex r(key + LR"(\s*=\s*[\"']([^\"']*)[\"'])", std::regex::icase); std::wsmatch m;
    return std::regex_search(tag,m,r)?m[2].str():L"";
}
static Analysis Analyze(const fs::path& file) {
    std::ifstream in(file, std::ios::binary); std::string bytes((std::istreambuf_iterator<char>(in)),{}); auto h=W(bytes); Analysis a;
    std::wsmatch tm; if(std::regex_search(h,tm,std::wregex(LR"(<title[^>]*>([\s\S]*?)</title>)",std::regex::icase))) a.title=HtmlText(tm[2].str());
    std::wregex tagR(LR"(<(/?)(header|nav|main|aside|section|article|div|p|h1|h2|h3|h4|button|input|textarea|select|img)[^>]*>)",std::regex::icase);
    std::wsregex_iterator it(h.begin(),h.end(),tagR), end; int depth=0;
    for(;it!=end;++it) { std::wstring raw=(*it)[0], tag; std::wsmatch nameMatch; std::regex_search(raw,nameMatch,std::wregex(LR"(<\s*/?\s*([a-zA-Z0-9]+))")); if(nameMatch.size()>1) tag=nameMatch[1].str(); std::transform(tag.begin(),tag.end(),tag.begin(),towlower); bool close=raw.size()>1 && raw[1]==L'/'; if(close) { depth=std::max(0,depth-1); continue; }
        Node n{tag,L"",Attr(raw,L"class"),Attr(raw,L"id"),Attr(raw,L"onclick"),depth}; auto next=it; ++next; size_t from=it->position()+it->length(), to=next==end?h.size():(size_t)next->position(); n.text=HtmlText(h.substr(from,to-from)); if(n.tag==L"input"||n.tag==L"textarea") n.text=Attr(raw,L"placeholder"); if(n.tag==L"img") n.text=Attr(raw,L"alt");
        if(!n.text.empty()||n.tag==L"button"||n.tag==L"input"||n.tag==L"textarea"||n.tag==L"select"||n.tag==L"img") a.nodes.push_back(n);
        if(n.tag==L"button")a.buttons++; if(n.tag==L"input"||n.tag==L"textarea"||n.tag==L"select")a.inputs++; if(n.tag.size()==2&&n.tag[0]==L'h')a.headings++;
        if(!raw.empty() && raw.back()!=L'/' && n.tag!=L"input" && n.tag!=L"img") depth++;
    } return a;
}
static std::wstring Esc(const std::wstring& s) { std::wstring r; for(wchar_t c:s) { if(c==L'\\'||c==L'\"')r+=L'\\'; if(c==L'\n'||c==L'\r')r+=L' '; else r+=c; } return r; }
static int Color(const std::wstring& cls, bool text=false) {
    if(cls.find(L"blue")!=std::wstring::npos||cls.find(L"indigo")!=std::wstring::npos) return text?0x00F5F5F5:0x00D87800;
    if(cls.find(L"green")!=std::wstring::npos||cls.find(L"emerald")!=std::wstring::npos) return text?0x00E8FFF1:0x0040A860;
    if(cls.find(L"purple")!=std::wstring::npos) return text?0x00FAF0FF:0x00A06080;
    if(cls.find(L"slate")!=std::wstring::npos||cls.find(L"gray")!=std::wstring::npos) return text?0x00E8E8E8:0x00302C28;
    return text?0x00E9E6E2:0x00231F1B;
}
static std::wstring Identifier(std::wstring value) {
    std::wstring result; bool upper=true;
    for(wchar_t c:value) { bool alpha=(c>=L'a'&&c<=L'z')||(c>=L'A'&&c<=L'Z')||(c>=L'0'&&c<=L'9'); if(!alpha){upper=true;continue;} if(upper&&c>=L'a'&&c<=L'z')c-=32; result+=c; upper=false; }
    if(result.empty()) result=L"UnnamedControl"; if(result[0]>=L'0'&&result[0]<=L'9') result=L"Control"+result; return result;
}
static std::wstring AgentBrief(const Analysis& a, const std::wstring& sourceName) {
    std::wstringstream o;
    o<<L"# KI-Arbeitsauftrag: "<<(a.title.empty()?sourceName:a.title)<<L"\n\n";
    o<<L"## Verbindliche Quellen\n\n- `mockup.html` ist die unveränderte visuelle und Interaktionsreferenz.\n- `"<<sourceName<<L"_template.cpp` startet die Fidelity-Ansicht.\n- Diese Datei beschreibt nur den Implementierungsauftrag; sie ersetzt weder HTML noch C++.\n\n";
    o<<L"## Architektur-Grenze\n\n- **GUI-Schicht:** Ereignisse entgegennehmen, Werte anzeigen, View-Model aktualisieren. Keine fachlichen Entscheidungen, Datenzugriffe oder Geschäftsregeln.\n- **Geschäftslogik:** eigenständige C++-Services/Use-Cases. Keine Win32-, Edge-, HTML- oder CSS-Abhängigkeit.\n- **Vertrag:** Die GUI ruft benannte Methoden auf; Services geben Ergebnisdaten/Fehler zurück. Das Design in `mockup.html` bleibt unverändert.\n\n";
    o<<L"## Erkannte interaktive Bedienelemente\n\n| Typ | Sichtbare Bezeichnung | Vorgeschlagener GUI-Handler | Geschäftslogik-Vertrag | HTML-Hinweis |\n|---|---|---|---|---|\n";
    int unnamed=1;
    for(const auto& n:a.nodes) {
        bool button=n.tag==L"button", input=n.tag==L"input"||n.tag==L"textarea", select=n.tag==L"select";
        if(!button&&!input&&!select) continue;
        std::wstring label=n.text.empty()?n.id:n.text; if(label.empty()) label=L"Unbenanntes Element "+std::to_wstring(unnamed++);
        std::wstring base=Identifier(n.id.empty()?label:n.id), handler, contract;
        if(button){handler=L"GuiController::On"+base+L"Clicked()"; contract=L"AppService::"+base+L"()";}
        else if(select){handler=L"GuiController::On"+base+L"Changed(value)"; contract=L"AppService::Set"+base+L"(value)";}
        else {handler=L"GuiController::On"+base+L"Changed(value)"; contract=L"ViewModel::"+base+L" = value";}
        o<<L"| `"<<n.tag<<L"` | "<<label<<L" | `"<<handler<<L"` | `"<<contract<<L"` | `"<<(n.id.empty()?n.classes:n.id)<<L"` |\n";
    }
    o<<L"\n## Implementierungsregeln\n\n1. Prüfe jeden Handler gegen `mockup.html`, bevor du ihn implementierst.\n2. Ergänze Funktionen zunächst als Interfaces/Services in der Geschäftslogik und verbinde sie danach mit dem GUI-Handler.\n3. Verändere HTML-Struktur, Tailwind-Klassen, Texte oder Layout nur auf ausdrückliche Produktanforderung.\n4. Dokumentiere neue Funktionen in dieser Tabelle oder einer zugehörigen Use-Case-Datei.\n";
    return o.str();
}
static std::wstring JsonEsc(const std::wstring& value) {
    std::wstring r; for(wchar_t c:value) { if(c==L'\\'||c==L'"') { r+=L'\\'; r+=c; } else if(c==L'\n'||c==L'\r') r+=L' '; else r+=c; } return r;
}
static std::wstring ImplementationMap(const Analysis& a, const std::wstring& sourceName) {
    std::wstringstream o; std::set<std::wstring> usedIds; int generated=1; bool first=true;
    o<<L"{\n  \"source\": \""<<JsonEsc(sourceName)<<L"\",\n  \"components\": [\n";
    for(const auto& n:a.nodes) {
        bool button=n.tag==L"button", input=n.tag==L"input"||n.tag==L"textarea", select=n.tag==L"select"; if(!button&&!input&&!select) continue;
        std::wstring label=n.text.empty()?n.id:n.text; std::wstring base=Identifier(n.id.empty()?label:n.id); std::wstring id=n.id.empty()?(button?L"btn-":(select?L"select-":L"input-"))+Identifier(label):n.id;
        id=Identifier(id); std::transform(id.begin(),id.end(),id.begin(),towlower); if(id.empty()) id=L"control"; while(usedIds.count(id)) id+=L"-"+std::to_wstring(generated++); usedIds.insert(id);
        std::wstring event=button?L"click":(select?L"change":L"input"); std::wstring handler=button?L"On"+base:((select?L"On":L"On")+base+L"Changed"); if(button) handler+=L"Clicked";
        if(!first)o<<L",\n"; first=false;
        o<<L"    {\n      \"id\": \""<<JsonEsc(id)<<L"\",\n      \"component\": \""<<n.tag<<L"\",\n      \"label\": \""<<JsonEsc(label)<<L"\",\n      \"event\": \""<<event<<L"\",\n      \"required_handler\": \""<<handler<<L"\",\n      \"status\": \"not_implemented\"\n    }";
    }
    o<<L"\n  ]\n}\n"; return o.str();
}
static std::wstring AgentHandoff(const Analysis& a, const std::wstring& stem, const std::wstring& html, const std::wstring& cpp, const std::wstring& brief, const std::wstring& map) {
    std::wstringstream o;
    o<<L"# Vollständige KI-Übergabe: "<<(a.title.empty()?stem:a.title)<<L"\n\n";
    o<<L"Diese Datei ist der vollständige Arbeitskontext. Sie enthält die Designreferenz, den C++-Startpunkt und den maschinenlesbaren Implementierungs-Backlog.\n\n";
    o<<L"## Verbindliche Regeln\n\n1. Das Design in `mockup.html` ist unverändert beizubehalten.\n2. Implementiere fachliche Funktionen hinter klaren GUI-Handlern; Geschäftslogik darf keine HTML-, CSS-, Edge- oder Win32-Abhängigkeit erhalten.\n3. Arbeite zuerst die Einträge in `IMPLEMENTATION_MAP.json` ab und ändere ihren Status erst nach funktionaler Prüfung.\n4. Erzeuge bei einer Dateiausgabe die vier unten enthaltenen Dateien wieder separat.\n\n";
    o<<L"## Datei: `"<<stem<<L"_template.cpp`\n\n```cpp\n"<<cpp<<L"```\n\n";
    o<<L"## Datei: `"<<stem<<L"_agent_brief.md`\n\n"<<brief<<L"\n\n";
    o<<L"## Datei: `IMPLEMENTATION_MAP.json`\n\n```json\n"<<map<<L"```\n\n";
    o<<L"## Datei: `mockup.html`\n\n```html\n"<<html<<L"\n```\n";
    return o.str();
}
static std::wstring Generated(const Analysis& a) {
    // Fidelity mode: keep the original browser document intact and generate a native C++ Edge app launcher.
    // The previous GDI renderer is intentionally bypassed because it cannot faithfully reproduce Tailwind CSS.
    std::wstringstream launcher;
    launcher << L"// Generated by MockupCppConverter - visual fidelity mode.\n"
              L"// Keep mockup.html next to this executable; it is rendered by the installed Microsoft Edge runtime.\n"
              L"#include <windows.h>\n#include <string>\n#include <algorithm>\n\n"
              L"int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {\n"
              L"  wchar_t exe[MAX_PATH]{}; GetModuleFileNameW(nullptr, exe, MAX_PATH);\n"
              L"  std::wstring folder(exe); size_t slash=folder.find_last_of(L\"\\\\/\"); if(slash!=std::wstring::npos) folder.resize(slash+1);\n"
              L"  std::wstring html=folder+L\"mockup.html\"; std::replace(html.begin(),html.end(),L'\\\\',L'/');\n"
              L"  std::wstring edge=L\"C:\\\\Program Files (x86)\\\\Microsoft\\\\Edge\\\\Application\\\\msedge.exe\";\n"
              L"  if(GetFileAttributesW(edge.c_str())==INVALID_FILE_ATTRIBUTES) edge=L\"C:\\\\Program Files\\\\Microsoft\\\\Edge\\\\Application\\\\msedge.exe\";\n"
              L"  std::wstring command=L\"\\\"\"+edge+L\"\\\" --app=\\\"file:///\"+html+L\"\\\"\";\n"
              L"  STARTUPINFOW si{}; si.cb=sizeof(si); PROCESS_INFORMATION pi{};\n"
              L"  if(!CreateProcessW(nullptr,command.data(),nullptr,nullptr,FALSE,0,nullptr,folder.c_str(),&si,&pi)) { MessageBoxW(nullptr,L\"Microsoft Edge konnte nicht gestartet werden. Bitte Edge/WebView2 installieren.\",L\"Mockup-Vorlage\",MB_ICONERROR); return 1; }\n"
              L"  CloseHandle(pi.hThread); CloseHandle(pi.hProcess); return 0;\n}\n";
    return launcher.str();
    std::wstringstream o; o<<L"// Generated by MockupCppConverter. No external libraries required.\n#include <windows.h>\n#include <vector>\n#include <string>\n\n";
    o<<L"struct Item { const wchar_t* text; const wchar_t* tag; const wchar_t* sourceClass; int color; };\nstatic const Item items[] = {\n";
    for(const auto& n:a.nodes) o<<L" { L\""<<Esc(n.text.empty()?n.tag:n.text)<<L"\", L\""<<n.tag<<L"\", L\""<<Esc(n.classes)<<L"\", 0x"<<std::hex<<Color(n.classes,n.tag==L"p"||n.tag[0]==L'h')<<std::dec<<L" },\n";
    o<<L"};\n\n// GDI wird zur Laufzeit geladen: der direkte KR-Compiler braucht dadurch keine -lgdi32 Option.\nstruct Gdi { HMODULE dll=LoadLibraryW(L\"gdi32.dll\"); decltype(&::CreateSolidBrush) brush=(decltype(brush))GetProcAddress(dll,\"CreateSolidBrush\"); decltype(&::DeleteObject) del=(decltype(del))GetProcAddress(dll,\"DeleteObject\"); decltype(&::SetBkMode) bk=(decltype(bk))GetProcAddress(dll,\"SetBkMode\"); decltype(&::GetStockObject) stock=(decltype(stock))GetProcAddress(dll,\"GetStockObject\"); decltype(&::CreateFontW) font=(decltype(font))GetProcAddress(dll,\"CreateFontW\"); decltype(&::SelectObject) select=(decltype(select))GetProcAddress(dll,\"SelectObject\"); decltype(&::SetTextColor) color=(decltype(color))GetProcAddress(dll,\"SetTextColor\"); } gdi;\n\nstatic LRESULT CALLBACK Proc(HWND w, UINT m, WPARAM p, LPARAM l) {\n if(m==WM_PAINT) { PAINTSTRUCT ps; HDC dc=BeginPaint(w,&ps); RECT r; GetClientRect(w,&r); HBRUSH bg=gdi.brush(RGB(18,19,22)); FillRect(dc,&r,bg); gdi.del(bg);\n int x=28,y=26,maxX=r.right-28; gdi.bk(dc,TRANSPARENT);\n for(const auto& e:items) { bool button=wcscmp(e.tag,L\"button\")==0, field=wcscmp(e.tag,L\"input\")==0||wcscmp(e.tag,L\"textarea\")==0||wcscmp(e.tag,L\"select\")==0; int h=button?38:(field?34:((e.tag[0]==L'h')?32:24)); if(y+h>r.bottom-24){x+=300;y=26;} RECT box={x,y,(x+270<maxX?x+270:maxX),y+h}; if(button||field){HBRUSH b=gdi.brush(button?RGB(0,120,212):RGB(35,37,43)); FillRect(dc,&box,b); gdi.del(b); FrameRect(dc,&box,(HBRUSH)gdi.stock(GRAY_BRUSH));}\n HFONT f=gdi.font((e.tag[0]==L'h')?20:14,0,0,0,(e.tag[0]==L'h')?FW_BOLD:FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,0,0,L\"Segoe UI\"); gdi.select(dc,f); gdi.color(dc,RGB(e.color&255,(e.color>>8)&255,(e.color>>16)&255)); RECT tx={x+10,y+4,box.right-6,y+h-2}; DrawTextW(dc,e.text,-1,&tx,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS); gdi.del(f); y+=h+9; } EndPaint(w,&ps); return 0;} if(m==WM_DESTROY)PostQuitMessage(0); return DefWindowProcW(w,m,p,l);}\n\nint WINAPI WinMain(HINSTANCE i,HINSTANCE, LPSTR,int){ WNDCLASSW c{};c.hInstance=i;c.lpszClassName=L\"GeneratedMockup\";c.lpfnWndProc=Proc;c.hCursor=LoadCursor(nullptr,IDC_ARROW);RegisterClassW(&c); HWND w=CreateWindowExW(0,c.lpszClassName,L\""<<Esc(a.title.empty()?L"Generated Mockup":a.title)<<L"\",WS_OVERLAPPEDWINDOW|WS_VISIBLE,100,80,1180,760,nullptr,nullptr,i,nullptr);MSG m;while(GetMessageW(&m,nullptr,0,0)){TranslateMessage(&m);DispatchMessageW(&m);} return 0; }\n"; return o.str();
}
static void Set(HWND h,const std::wstring& s){SetWindowTextW(h,s.c_str());}
static std::wstring Get(HWND h){int n=GetWindowTextLengthW(h);std::wstring s(n,L' ');GetWindowTextW(h,s.data(),n+1);return s;}
static void Browse(HWND target,bool folder){ wchar_t path[MAX_PATH]={}; if(folder){ BROWSEINFOW b{}; b.hwndOwner=GetActiveWindow(); b.lpszTitle=L"Ausgabeordner auswählen"; PIDLIST_ABSOLUTE id=SHBrowseForFolderW(&b); if(id){SHGetPathFromIDListW(id,path);CoTaskMemFree(id);} }else{ OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=GetActiveWindow();o.lpstrFilter=L"HTML-Dateien\0*.html;*.htm\0Alle Dateien\0*.*\0";o.lpstrFile=path;o.nMaxFile=MAX_PATH;o.Flags=OFN_FILEMUSTEXIST;if(!GetOpenFileNameW(&o))return;} if(path[0])Set(target,path); }
static void Convert(){ try { auto file=fs::path(Get(g_file)); auto out=fs::path(Get(g_output)); if(!fs::is_regular_file(file)){Set(g_status,L"Bitte zuerst eine gültige HTML-Datei wählen.");return;} if(out.empty())out=file.parent_path(); std::error_code ec;fs::create_directories(out,ec); if(ec){Set(g_status,std::wstring(L"Ausgabeordner kann nicht erstellt werden: ")+W(ec.message()));return;} std::ifstream source(file,std::ios::binary);std::string sourceBytes((std::istreambuf_iterator<char>(source)),{});fs::copy_file(file,out/L"mockup.html",fs::copy_options::overwrite_existing,ec); if(ec){Set(g_status,std::wstring(L"HTML-Datei konnte nicht kopiert werden: ")+W(ec.message()));return;} auto a=Analyze(file); auto stem=file.stem().wstring(); auto cpp=Generated(a), briefText=AgentBrief(a,stem), mapText=ImplementationMap(a,stem); auto dest=out/(stem+L"_template.cpp");std::ofstream f(dest,std::ios::binary);if(!f){Set(g_status,L"Ausgabedatei kann nicht geschrieben werden.");return;}f<<U(cpp);std::ofstream brief(out/(stem+L"_agent_brief.md"),std::ios::binary);if(!brief){Set(g_status,L"Agenten-Arbeitsauftrag kann nicht geschrieben werden.");return;}brief<<U(briefText);std::ofstream map(out/L"IMPLEMENTATION_MAP.json",std::ios::binary);if(!map){Set(g_status,L"Implementierungs-Map kann nicht geschrieben werden.");return;}map<<U(mapText);std::ofstream handoff(out/L"AI_HANDOFF.md",std::ios::binary);if(!handoff){Set(g_status,L"KI-Übergabe kann nicht geschrieben werden.");return;}handoff<<U(AgentHandoff(a,stem,W(sourceBytes),cpp,briefText,mapText));Set(g_status,L"Erstellt: C++-Vorlage, HTML, Brief, Map und AI_HANDOFF.md.");Set(g_stats,L"Analyse: "+std::to_wstring(a.nodes.size())+L" sichtbare Elemente  |  "+std::to_wstring(a.buttons)+L" Buttons  |  "+std::to_wstring(a.inputs)+L" Eingaben");} catch(const std::exception& e) { Set(g_status,std::wstring(L"Analysefehler: ")+W(e.what())); } }
static LRESULT CALLBACK MainProc(HWND w,UINT m,WPARAM p,LPARAM l){if(m==WM_COMMAND){int id=LOWORD(p);if(id==1)Browse(g_file,false);if(id==2)Browse(g_output,true);if(id==3)Convert();return 0;}if(m==WM_DESTROY)PostQuitMessage(0);return DefWindowProcW(w,m,p,l);}
int WINAPI wWinMain(HINSTANCE h,HINSTANCE,PWSTR,int){WNDCLASSW c{};c.hInstance=h;c.lpszClassName=L"MockupCppConverter";c.lpfnWndProc=MainProc;c.hCursor=LoadCursor(nullptr,IDC_ARROW);c.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);RegisterClassW(&c);HWND w=CreateWindowExW(0,c.lpszClassName,L"Mockup → C++ Converter",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_VISIBLE,260,180,720,350,nullptr,nullptr,h,nullptr);CreateWindowW(L"STATIC",L"HTML-Mockup in eine native C++/Win32-GUI-Vorlage übertragen",WS_CHILD|WS_VISIBLE,24,22,620,25,w,nullptr,h,nullptr);CreateWindowW(L"STATIC",L"Mockup-Datei",WS_CHILD|WS_VISIBLE,24,62,130,22,w,nullptr,h,nullptr);g_file=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"H:\\engine-develop\\mockups\\aura_workspace_mockup.html",WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,24,85,535,25,w,nullptr,h,nullptr);CreateWindowW(L"BUTTON",L"Durchsuchen…",WS_CHILD|WS_VISIBLE,570,85,115,25,w,(HMENU)1,h,nullptr);CreateWindowW(L"STATIC",L"Ausgabeordner",WS_CHILD|WS_VISIBLE,24,122,150,22,w,nullptr,h,nullptr);g_output=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"H:\\engine-develop\\MockupCppConverter\\generated",WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,24,145,535,25,w,nullptr,h,nullptr);CreateWindowW(L"BUTTON",L"Ordner…",WS_CHILD|WS_VISIBLE,570,145,115,25,w,(HMENU)2,h,nullptr);CreateWindowW(L"BUTTON",L"C++ Vorlage erzeugen",WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,24,196,210,35,w,(HMENU)3,h,nullptr);g_stats=CreateWindowW(L"STATIC",L"Bereit zur Analyse.",WS_CHILD|WS_VISIBLE,24,245,650,22,w,nullptr,h,nullptr);g_status=CreateWindowW(L"STATIC",L"Erzeugt eine einzelne, editierbare .cpp-Datei ohne externe Bibliotheken.",WS_CHILD|WS_VISIBLE,24,272,650,35,w,nullptr,h,nullptr);MSG q;while(GetMessageW(&q,nullptr,0,0)){TranslateMessage(&q);DispatchMessageW(&q);} return 0; }
