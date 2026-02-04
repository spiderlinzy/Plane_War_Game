#include <iostream>
#include <graphics.h> // EasyX图形库头文件
#include <vector>
#include<fstream>
#include<sstream>
#include <conio.h>    // 用于键盘输入
#include <ctime>      // 用于随机数生成
using namespace std;

// 游戏窗口尺寸
constexpr auto swidth = 1700;    // 窗口宽度
constexpr auto sheight = 1000;  // 窗口高度
constexpr auto DATA_FILE = _T("gamedata.dat");
constexpr unsigned int SHP = 4; // 玩家飞机生命值
int propSpawnTimer = 0;
const int PROP_SPAWN_INTERVAL = 500;
static unsigned long long currentDiamonds = 0;  // 本局获得的钻石
static unsigned long long totalDiamonds = 0;    // 累计总钻石
constexpr int hurttime = 1000;
constexpr int hurttime2 = 1000;// 受伤冷却时间(毫秒)
static int unlockedLevel = 1;

void putimagePNG(int x, int y, IMAGE* picture) {
    // 变量初始化
    DWORD* dst = GetImageBuffer();    // Get the pointer to the screen buffer
    DWORD* draw = GetImageBuffer(picture);  // Get the pointer to the picture buffer
    int picture_width = picture->getwidth();  // Get the width of the picture
    int picture_height = picture->getheight();  // Get the height of the picture
    int screen_width = getwidth();  // Get the width of the screen
    int screen_height = getheight();  // Get the height of the screen

    for (int iy = 0; iy < picture_height; iy++) {
        for (int ix = 0; ix < picture_width; ix++) {
            if (ix + x >= 0 && ix + x < screen_width && iy + y >= 0 && iy + y < screen_height) {
                // Get the alpha value
                int alpha = (draw[iy * picture_width + ix] & 0xff000000) >> 24;
                // Get the RGB values
                int blue = (draw[iy * picture_width + ix] & 0x00ff0000) >> 16;
                int green = (draw[iy * picture_width + ix] & 0x0000ff00) >> 8;
                int red = draw[iy * picture_width + ix] & 0x000000ff;

                // Calculate the new RGB values with alpha blending
                dst[(iy + y) * screen_width + (ix + x)] = ((dst[(iy + y) * screen_width + (ix + x)] & 0x00ffffff) * (255 - alpha) + (red << 16) * alpha + (green << 8) * alpha + blue * alpha) / 255;
            }
        }
    }
}

void clearMouseEvents() {
    ExMessage msg;
    while (peekmessage(&msg, EM_MOUSE)) {
        // 忽略所有鼠标事件
    }
}
int numberOfPlayers = 0;
static int diamondSpawnTimer = 0;  // 钻石生成计时器
const int DIAMOND_MIN_DELAY = 1000;  // 最小间隔
const int DIAMOND_MAX_DELAY = 1500; // 最大间隔

// 准备界面
enum StartProp { NONE, SHIELD, HP_BOOST, STRENGTH };
static StartProp selectedProp = NONE;  // 默认无道具
const int PROP_ICON_SIZE = 80;
static int propCount[3] = { 0 }; // 0=护盾 1=生命 2=强化
const int PROP_COST = 3;       // 道具单价

int ModeNumber = 0;
constexpr int MAX_LEVELS = 3;


void Welcome1();
void shop();
void Welcome2();
void Welcome3();
void Over(unsigned long long& kill, clock_t& startTime);
void Victory(unsigned long long& kill, int stage);
bool PointInRect(int x, int y, RECT& r)
{
    return (r.left <= x && x <= r.right && r.top <= y && y <= r.bottom);
}
void Welcome1() {
    LPCTSTR title = _T("飞机大战");
    LPCTSTR single = _T("单人模式");
    LPCTSTR doublePlayer = _T("双人模式");

    RECT singler, doublePlayerr;

    BeginBatchDraw();
    setbkcolor(RGB(240, 240, 240)); // 浅灰色背景
    cleardevice();

    settextstyle(80, 0, _T("黑体"), 0, 0, FW_BOLD, false, false, false); // 加粗字体
    settextcolor(BLACK);
    outtextxy(swidth / 2 - textwidth(title) / 2, sheight / 5, title);

    settextstyle(40, 0, _T("黑体"));
    singler.left = swidth / 2 - textwidth(single) / 2;
    singler.right = singler.left + textwidth(single);
    singler.top = sheight / 5 * 2.5;
    singler.bottom = singler.top + textheight(single);

    doublePlayerr.left = swidth / 2 - textwidth(doublePlayer) / 2;
    doublePlayerr.right = doublePlayerr.left + textwidth(doublePlayer);
    doublePlayerr.top = sheight / 5 * 3;
    doublePlayerr.bottom = doublePlayerr.top + textheight(doublePlayer);

    // 绘制按钮
    setfillcolor(RGB(100, 150, 200));
    fillroundrect(singler.left - 10, singler.top - 10, singler.right + 10, singler.bottom + 10, 10, 10);
    fillroundrect(doublePlayerr.left - 10, doublePlayerr.top - 10, doublePlayerr.right + 10, doublePlayerr.bottom + 10, 10, 10);

    settextcolor(WHITE);
    setbkmode(TRANSPARENT);
    outtextxy(singler.left, singler.top, single);
    outtextxy(doublePlayerr.left, doublePlayerr.top, doublePlayer);

    EndBatchDraw();

    while (true) {
        ExMessage mess;
        getmessage(&mess, EM_MOUSE);
        if (mess.lbutton) {
            if (PointInRect(mess.x, mess.y, singler)) {
                numberOfPlayers = 1;
                clearMouseEvents();
                return;
            }
            else if (PointInRect(mess.x, mess.y, doublePlayerr)) {
                numberOfPlayers = 2;
                clearMouseEvents();
                return;
            }
        }
    }
}

void SaveGameData() {
    ofstream file(DATA_FILE);
    if (file.is_open()) {
        // 第一行，钻石
        file << totalDiamonds << endl;

        // 第二行，道具数量，护盾，血量，强化
        file << propCount[0] << ","
            << propCount[1] << ","
            << propCount[2] << endl;

        file << unlockedLevel << endl;

        file.close();
    }
}

// 加载游戏数据
void LoadGameData() {
    ifstream file(DATA_FILE);
    if (file.is_open()) {
        string line;

        // 读取钻石
        if (getline(file, line)) {
            istringstream iss(line);
            iss >> totalDiamonds;
        }

        // 读取道具数量
        if (getline(file, line)) {
            istringstream iss(line);
            char comma;
            iss >> propCount[0] >> comma
                >> propCount[1] >> comma
                >> propCount[2];
        }

        if (getline(file, line)) {
            istringstream iss3(line);
            iss3 >> unlockedLevel;
        }

        file.close();
    }
    else {
        // 文件不存在，使用默认值
        totalDiamonds = 0;
        propCount[0] = propCount[1] = propCount[2] = 0;
        unlockedLevel = 1;
    }
}
StartProp ShowPrepareInterface() {
    const int ICON_SIZE = 80;
    const int PADDING = 20;

    // 加载道具图标
    IMAGE icons[3];
    loadimage(&icons[0], _T("../images/shield.png"), ICON_SIZE, ICON_SIZE);
    loadimage(&icons[1], _T("../images/heart.png"), ICON_SIZE, ICON_SIZE);
    loadimage(&icons[2], _T("../images/strength.png"), ICON_SIZE, ICON_SIZE);

    // 初始化选择状态
    StartProp selected = NONE;
    RECT propRects[3];
    bool isConfirmed = false;

    // 主循环
    while (!isConfirmed) {
        BeginBatchDraw();
        cleardevice();

        // 绘制提示文字
        settextcolor(BLACK);
        setlinecolor(RGB(240, 240, 240));
        settextstyle(40, 0, _T("黑体"));
        LPCTSTR tip = _T("请选择初始道具");
        outtextxy(swidth / 2 - textwidth(tip) / 2, sheight / 6, tip);

        // 绘制道具选项
        int startX = (swidth - (ICON_SIZE * 3 + PADDING * 2)) / 2;
        for (int i = 0; i < 3; ++i) {
            int x = startX + i * (ICON_SIZE + PADDING);
            int y = sheight / 2 - ICON_SIZE / 2;

            // 绘制图标
            putimagePNG(x, y, &icons[i]);
            if (selected == static_cast<StartProp>(i + 1)) {
                setlinecolor(RED);
                setlinestyle(PS_SOLID, 1);
                rectangle(x - 5, y - 5, x + ICON_SIZE + 5, y + ICON_SIZE + 5);
            }
            else {
                setlinecolor(RGB(240, 240, 240));
                setlinestyle(PS_SOLID, 1);
                rectangle(x - 5, y - 5, x + ICON_SIZE + 5, y + ICON_SIZE + 5);
            }
            // 绘制数量背景
            setfillcolor(RGB(240, 240, 240));
            solidroundrect(x - 5, y + ICON_SIZE + 10,
                x + ICON_SIZE + 5, y + ICON_SIZE + 30, 8, 8);

            // 绘制数量文本
            TCHAR countText[16];
            _stprintf_s(countText, _T("×%d"), propCount[i]);
            settextcolor(RGB(80, 80, 80));
            settextstyle(18, 0, _T("黑体"));
            outtextxy(
                x + (ICON_SIZE - textwidth(countText)) / 2,
                y + ICON_SIZE + 10,
                countText
            );
            propRects[i] = { x, y, x + ICON_SIZE, y + ICON_SIZE };
            setlinecolor(RGB(240, 240, 240));
        }

        // 绘制确认按钮
        setfillcolor(RGB(0, 200, 0));
        RECT confirmBtn = { swidth / 2 - 60, sheight * 3 / 4, swidth / 2 + 60, sheight * 3 / 4 + 50 };
        fillroundrect(confirmBtn.left, confirmBtn.top, confirmBtn.right, confirmBtn.bottom, 10, 10);
        settextcolor(BLACK);
        settextstyle(40, 0, _T("黑体"));
        setbkmode(TRANSPARENT);
        outtextxy(confirmBtn.left + 20, confirmBtn.top + 5, _T("确认"));

        EndBatchDraw();

        // 处理输入
        ExMessage msg;
        while (peekmessage(&msg, EM_MOUSE)) {
            if (msg.message == WM_LBUTTONDOWN) {
                // 检测道具选择
                for (int i = 0; i < 3; ++i) {
                    if (PointInRect(msg.x, msg.y, propRects[i])) {
                        // 如果点击的是已选中的道具，则取消选中
                        if (selected == static_cast<StartProp>(i + 1)) {
                            // 返还道具数量
                            propCount[i]++;
                            selected = NONE;  // 取消选中
                            continue;         // 跳过后续处理
                        }
                        // 记录当前选择的道具索引
                        int clickedIndex = i;
                        // 检查道具数量
                        if (propCount[clickedIndex] <= 0) {
                            // 弹出购买提示
                            TCHAR msgText[128];
                            _stprintf_s(msgText,
                                _T("道具数量不足，是否花费3钻石购买？\n总钻石：%llu"),
                                totalDiamonds);

                            int ret = MessageBox(GetHWnd(), msgText,
                                _T("提示"), MB_YESNO | MB_ICONQUESTION);

                            if (ret == IDYES) {
                                if (totalDiamonds >= PROP_COST) {
                                    totalDiamonds -= PROP_COST;
                                    propCount[clickedIndex]++;

                                    // 购买成功后立即选中该道具
                                    // 先返还之前选中的道具（如果有）
                                    if (selected != NONE) {
                                        int prevIndex = static_cast<int>(selected) - 1;
                                        propCount[prevIndex]++;
                                    }

                                    // 扣除新道具数量并选中
                                    propCount[clickedIndex]--;
                                    selected = static_cast<StartProp>(clickedIndex + 1);
                                }
                                else {
                                    MessageBox(GetHWnd(),
                                        _T("钻石数量不足"),
                                        _T("购买失败"), MB_OK);
                                }
                            }
                            continue; // 跳过后续处理
                        }
                        // 如果之前已选中其他道具，先返还其数量
                        if (selected != NONE) {
                            // 返还之前选中道具的数量
                            int prevIndex = static_cast<int>(selected) - 1;
                            propCount[prevIndex]++;
                        }

                        // 扣除数量并选择
                        propCount[clickedIndex]--;
                        selected = static_cast<StartProp>(clickedIndex + 1);
                    }
                }
                // 检测确认按钮
                if (PointInRect(msg.x, msg.y, confirmBtn)) {
                    isConfirmed = true;
                }
            }
        }
        Sleep(10);
    }
    SaveGameData();
    return selected;
}

void shop() {
    // 加载道具图片
    IMAGE shieldImg, heartImg, strengthImg;
    loadimage(&shieldImg, _T("../images/shield.png"), 100, 100); // 增大图标尺寸
    loadimage(&heartImg, _T("../images/heart.png"), 100, 100);
    loadimage(&strengthImg, _T("../images/strength.png"), 100, 100);

    // 标题
    LPCTSTR title = _T("商店");
    LPCTSTR exitShop = _T("退出商店");

    // 道具名称
    LPCTSTR propNames[] = { _T("护盾"), _T("血量加成"), _T("子弹强化") };

    // 矩形区域
    RECT propRects[3];
    RECT buyButtons[3];
    RECT exitShopr;

    // 道具价格
    const int propPrices[] = { 3, 3, 3 }; // 每个道具的价格

    while (true) {
        BeginBatchDraw();
        setbkcolor(RGB(240, 240, 240)); // 浅灰色背景
        cleardevice();

        // 设置标题样式
        settextstyle(80, 0, _T("黑体"), 0, 0, FW_BOLD, false, false, false); // 加粗字体
        settextcolor(RGB(50, 50, 50)); // 深灰色文字
        outtextxy(swidth / 2 - textwidth(title) / 2, sheight / 10, title);

        // 道具图片位置
        int startX = (swidth - (100 * 3 + 40 * 2)) / 2; // 调整间距
        int yPos = sheight / 5 * 2;

        for (int i = 0; i < 3; i++) {
            propRects[i] = { startX + i * (100 + 40), yPos,
                            startX + i * (100 + 40) + 100, yPos + 100 };

            // 购买按钮位置，下移一点
            buyButtons[i] = { propRects[i].left, propRects[i].bottom + 30,
                             propRects[i].right, propRects[i].bottom + 60 };
        }

        // 退出按钮位置，左移一点
        exitShopr = { swidth - textwidth(exitShop) - 60, 20,
                     swidth - 60, 20 + textheight(exitShop) };

        // 绘制道具图片
        putimagePNG(propRects[0].left, propRects[0].top, &shieldImg);
        putimagePNG(propRects[1].left, propRects[1].top, &heartImg);
        putimagePNG(propRects[2].left, propRects[2].top, &strengthImg);

        // 显示道具名称和价格
        settextstyle(25, 0, _T("黑体"));
        for (int i = 0; i < 3; i++) {
            // 道具名称
            outtextxy(propRects[i].left + (100 - textwidth(propNames[i])) / 2,
                propRects[i].top - 30, propNames[i]);

            // 价格
            TCHAR priceText[20];
            _stprintf_s(priceText, _T("%d颗钻石/个"), propPrices[i]);
            outtextxy(propRects[i].left + (100 - textwidth(priceText)) / 2,
                propRects[i].top - 60, priceText);
        }

        // 显示拥有的道具数量
        settextcolor(RGB(80, 80, 80));
        for (int i = 0; i < 3; i++) {
            TCHAR countText[16];
            _stprintf_s(countText, _T("数量:%d"), propCount[i]);
            outtextxy(propRects[i].left + (100 - textwidth(countText)) / 2,
                propRects[i].bottom + 5, countText);
        }

        // 绘制购买按钮
        setfillcolor(RGB(100, 200, 100));
        for (int i = 0; i < 3; i++) {
            fillroundrect(buyButtons[i].left, buyButtons[i].top,
                buyButtons[i].right, buyButtons[i].bottom, 10, 10);

            // 按钮显示"购买"文字
            LPCTSTR buyText = _T("购买");
            settextcolor(WHITE);
            setbkmode(TRANSPARENT);
            outtextxy(buyButtons[i].left + (buyButtons[i].right - buyButtons[i].left - textwidth(buyText)) / 2,
                buyButtons[i].top + (buyButtons[i].bottom - buyButtons[i].top - textheight(buyText)) / 2,
                buyText);
        }

        // 绘制退出按钮
        setfillcolor(RGB(200, 100, 100));
        fillroundrect(exitShopr.left, exitShopr.top, exitShopr.right, exitShopr.bottom, 10, 10);
        settextcolor(WHITE);
        outtextxy(exitShopr.left + (exitShopr.right - exitShopr.left - textwidth(exitShop)) / 2,
            exitShopr.top + (exitShopr.bottom - exitShopr.top - textheight(exitShop)) / 2,
            exitShop);

        // 显示钻石数量
        TCHAR diamondText[50];
        _stprintf_s(diamondText, _T("钻石: %llu"), totalDiamonds);
        settextstyle(25, 0, _T("黑体"));
        outtextxy(20, 20, diamondText);

        // 设置线条颜色和样式
        setlinecolor(RGB(200, 200, 200)); // 浅灰色边框
        setlinestyle(PS_SOLID, 2);

        // 绘制道具边框
        for (int i = 0; i < 3; i++) {
            rectangle(propRects[i].left, propRects[i].top, propRects[i].right, propRects[i].bottom);
        }

        // 绘制按钮边框
        for (int i = 0; i < 3; i++) {
            rectangle(buyButtons[i].left, buyButtons[i].top, buyButtons[i].right, buyButtons[i].bottom);
        }
        rectangle(exitShopr.left, exitShopr.top, exitShopr.right, exitShopr.bottom);

        EndBatchDraw();

        // 处理鼠标事件
        ExMessage msg;
        while (peekmessage(&msg, EM_MOUSE)) {
            if (msg.message == WM_LBUTTONDOWN) {
                // 点击退出按钮
                if (PointInRect(msg.x, msg.y, exitShopr)) {
                    clearMouseEvents();
                    return;
                }

                // 处理购买按钮
                for (int i = 0; i < 3; i++) {
                    if (PointInRect(msg.x, msg.y, buyButtons[i])) {
                        // 提示用户输入购买数量
                        TCHAR inputBuffer[10] = { 0 };
                        TCHAR prompt[100];
                        _stprintf_s(prompt, _T("请输入数量 (钻石数: %llu):"), totalDiamonds);

                        // 使用InputBox获取用户输入
                        if (InputBox(inputBuffer, 10, prompt, _T("购买道具"), propNames[i], 0, 0, true) == IDOK) {
                            // 将输入转换为整数
                            int buyCount = _ttoi(inputBuffer);

                            if (buyCount <= 0) {
                                MessageBox(GetHWnd(), _T("输入无效，请重新输入"), _T("错误"), MB_OK | MB_ICONERROR);
                                continue;
                            }

                            // 计算总价
                            unsigned long long totalCost = static_cast<unsigned long long>(buyCount) * propPrices[i];

                            if (totalDiamonds < totalCost) {
                                TCHAR errorMsg[100];
                                _stprintf_s(errorMsg, _T("钻石不足! "));
                                MessageBox(GetHWnd(), errorMsg, _T("购买失败"), MB_OK | MB_ICONWARNING);
                                continue;
                            }

                            // 确认购买
                            TCHAR confirmMsg[150];
                            _stprintf_s(confirmMsg, _T("确认购买 %d 个%s，总价 %llu 颗钻石?"), buyCount, propNames[i], totalCost);

                            if (MessageBox(GetHWnd(), confirmMsg, _T("确认购买"), MB_YESNO | MB_ICONQUESTION) == IDYES) {
                                // 扣除钻石
                                totalDiamonds -= totalCost;

                                // 增加道具数量
                                propCount[i] += buyCount;

                                // 保存游戏数据
                                SaveGameData();

                                // 显示购买成功消息
                                TCHAR successMsg[100];
                                _stprintf_s(successMsg, _T("购买成功! 获得 %d 个%s"), buyCount, propNames[i]);
                                MessageBox(GetHWnd(), successMsg, _T("购买成功"), MB_OK | MB_ICONINFORMATION);
                            }
                        }
                    }
                }
            }
        }

        Sleep(10);
    }
    SaveGameData();
}
void Welcome2() {
    LPCTSTR enterShop = _T("进入商店");
    LPCTSTR endlessMode = _T("无尽模式");
    LPCTSTR selectLevelMode = _T("选择关卡");

    RECT enterShopr, endlessModer, selectLevelModer;

    while (true) {
        BeginBatchDraw();
        setbkcolor(RGB(240, 240, 240)); // 浅灰色背景
        cleardevice();

        settextstyle(40, 0, _T("黑体"));
        enterShopr.left = swidth / 2 - textwidth(enterShop) / 2;
        enterShopr.right = enterShopr.left + textwidth(enterShop);
        enterShopr.top = sheight / 5 * 2;
        enterShopr.bottom = enterShopr.top + textheight(enterShop);

        endlessModer.left = swidth / 2 - textwidth(endlessMode) / 2;
        endlessModer.right = endlessModer.left + textwidth(endlessMode);
        endlessModer.top = sheight / 5 * 2.5;
        endlessModer.bottom = endlessModer.top + textheight(endlessMode);

        selectLevelModer.left = swidth / 2 - textwidth(selectLevelMode) / 2;
        selectLevelModer.right = selectLevelModer.left + textwidth(selectLevelMode);
        selectLevelModer.top = sheight / 5 * 3;
        selectLevelModer.bottom = selectLevelModer.top + textheight(selectLevelMode);

        // 绘制按钮
        setfillcolor(RGB(100, 150, 200));
        fillroundrect(enterShopr.left - 10, enterShopr.top - 10, enterShopr.right + 10, enterShopr.bottom + 10, 10, 10);
        fillroundrect(endlessModer.left - 10, endlessModer.top - 10, endlessModer.right + 10, endlessModer.bottom + 10, 10, 10);
        fillroundrect(selectLevelModer.left - 10, selectLevelModer.top - 10, selectLevelModer.right + 10, selectLevelModer.bottom + 10, 10, 10);

        settextcolor(WHITE);
        outtextxy(enterShopr.left, enterShopr.top, enterShop);
        outtextxy(endlessModer.left, endlessModer.top, endlessMode);
        outtextxy(selectLevelModer.left, selectLevelModer.top, selectLevelMode);

        EndBatchDraw();

        ExMessage mess;
        getmessage(&mess, EM_MOUSE);
        if (mess.lbutton) {
            if (PointInRect(mess.x, mess.y, enterShopr)) {
                clearMouseEvents();
                shop();
            }
            else if (PointInRect(mess.x, mess.y, endlessModer)) {
                ModeNumber = 0;
                clearMouseEvents();
                return;
            }
            else if (PointInRect(mess.x, mess.y, selectLevelModer)) {
                ModeNumber = 1; // 选择模式号
                clearMouseEvents();
                Welcome3(); // 进入选择关卡
                return;
            }
        }
    }
}
void Welcome3() {
    LPCTSTR levels[] = { _T("第一关"), _T("第二关"), _T("第三关") };
    RECT levelRects[MAX_LEVELS];
    const COLORREF lockedColor = RGB(180, 180, 180); // 灰色表示未解锁

    while (true) {
        BeginBatchDraw();
        cleardevice();
        setbkcolor(RGB(240, 240, 240));
        settextstyle(40, 0, _T("黑体"));

        for (int i = 0; i < MAX_LEVELS; i++) {
            int centerX = swidth / 2 - textwidth(levels[i]) / 2;
            int yPos = sheight / 5 * 2 + i * 60;

            levelRects[i] = {
                centerX - 10, yPos - 10,
                centerX + textwidth(levels[i]) + 10,
                yPos + textheight(levels[i]) + 10
            };

            // 根据解锁状态设置颜色
            if (i + 1 <= unlockedLevel) {
                setfillcolor(RGB(100, 150, 200)); // 可选择的蓝色
                settextcolor(WHITE);
            }
            else {
                setfillcolor(lockedColor); // 未解锁的灰色
                settextcolor(RGB(100, 100, 100));
            }

            fillroundrect(levelRects[i].left, levelRects[i].top,
                levelRects[i].right, levelRects[i].bottom, 10, 10);

            outtextxy(centerX, yPos, levels[i]);

            // 添加锁定图标
            if (i + 1 > unlockedLevel) {
                settextstyle(20, 0, _T("黑体"));
                LPCTSTR locked = _T("(锁定)");
                outtextxy(levelRects[i].right + 10,
                    yPos + (textheight(levels[i]) - textheight(locked)) / 2,
                    locked);
                settextstyle(40, 0, _T("黑体"));
            }
        }

        EndBatchDraw();

        ExMessage mess;
        while (peekmessage(&mess, EM_MOUSE)) {
            if (mess.message == WM_LBUTTONDOWN) {
                for (int i = 0; i < MAX_LEVELS; i++) {
                    // 只允许点击已解锁关卡
                    if (PointInRect(mess.x, mess.y, levelRects[i]) &&
                        (i + 1 <= unlockedLevel))
                    {
                        ModeNumber = i + 1;
                        clearMouseEvents();
                        return;
                    }
                }
            }
        }
        Sleep(10);
    }
}
void Over(unsigned long long& kill, clock_t& startTime)
{
    clock_t endTime = clock();
    double survivalTime = (double)(endTime - startTime) / CLOCKS_PER_SEC;

    double score = survivalTime + kill * 10;

    TCHAR* str = new TCHAR[128];
    _stprintf_s(str, 128, _T("杀敌数量%llu"), kill);

    settextcolor(RED);

    outtextxy(swidth / 2 - textwidth(str) / 2, sheight / 5, str);

    TCHAR* survivalStr = new TCHAR[128];
    _stprintf_s(survivalStr, 128, _T("生存时间：%.2f 秒"), survivalTime);
    outtextxy(swidth / 2 - textwidth(survivalStr) / 2, sheight / 5 + 60, survivalStr);

    TCHAR* scoreStr = new TCHAR[128];
    _stprintf_s(scoreStr, 128, _T("得分：%.2f"), score);

    outtextxy(swidth / 2 - textwidth(scoreStr) / 2, sheight / 5 + 120, scoreStr);

    currentDiamonds += kill / 150;
    totalDiamonds += kill / 150;

    // 文字高度
    int textH = textheight(_T("得分"));

    // 当前钻石
    TCHAR* currentStr = new TCHAR[128];
    _stprintf_s(currentStr, 128, _T("当前钻石: %llu"), currentDiamonds);
    outtextxy(swidth / 2 - textwidth(currentStr) / 2, sheight / 2, currentStr);

    // 累计钻石
    TCHAR* totalStr = new TCHAR[128];
    _stprintf_s(totalStr, 128, _T("累计钻石: %llu"), totalDiamonds);
    outtextxy(swidth / 2 - textwidth(totalStr) / 2, sheight / 2 + textH + 10, totalStr); // +10 为间距


    settextcolor(RED);
    outtextxy(swidth / 2 - textwidth(str) / 2, sheight / 5, str);

    currentDiamonds = 0;

    outtextxy(swidth / 2 - textwidth(scoreStr) / 2, sheight / 5 + 120, scoreStr);

    // 显示提示信息，按Enter继续
    LPCTSTR info = _T("按Enter继续");
    settextstyle(20, 0, _T("黑体"));

    outtextxy(swidth / 2 - textwidth(info) / 2, sheight / 5 + 180, info);

    // 显示提示信息，按Esc退出游戏
    LPCTSTR escInfo = _T("按Esc退出游戏");
    outtextxy(swidth / 2 - textwidth(escInfo) / 2, sheight / 5 + 210, escInfo);

    while (true)
    {
        ExMessage mess;
        getmessage(&mess, EM_KEY);
        if (mess.vkcode == 0x0D)
        {
            SaveGameData();
            return;
        }
        else if (mess.vkcode == VK_ESCAPE) { // Esc键
            SaveGameData();
            exit(0);
        }
    }
}
// 敌机类型定义
enum EnemyType {
    NORMAL,    // 普通飞机
    SUICIDE,   // 自爆飞机
    TANK,      // 高血飞机
    FAST,      // 高速飞机
    BOSS       // Boss飞机
};



// 判断两个矩形是否碰撞
bool RectDuangRect(const RECT& r1, RECT& r2)
{
    RECT r;
    r.left = r1.left - (r2.right - r2.left);
    r.right = r1.right;
    r.top = r1.top - (r2.bottom - r2.top);
    r.bottom = r1.bottom;
    return (r.left < r2.left && r2.left <= r.right && r.top <= r2.top && r2.top <= r.bottom);
}

class Prop {
public:
    Prop(IMAGE& img, int type, int x, int y)
        : img(img), type(type), x(x), y(y), active(true) {
    }

    // 更新位置
    bool Update() {
        y += 2; // 下落速度
        return y < sheight; // 超出屏幕返回false
    }

    // 绘制道具
    void Show() {
        putimagePNG(x, y, &img);
    }

    // 获取碰撞矩形
    RECT GetRect() {
        return { x, y, x + img.getwidth(), y + img.getheight() };
    }

    int GetType() const { return type; }
    bool IsActive() const { return active; }
    void Collect() { active = false; }

private:
    IMAGE& img; // 道具图片引用
    int type;
    int x, y;   // 位置
    bool active; // 是否有效
};
// 基类Object
class Object {
protected:
    RECT rect;       // 对象位置矩形
    unsigned int HP; // 生命值
    bool isdie;      // 是否死亡标志

public:
    Object(int x, int y, int width, int height, unsigned int hp)
        : HP(hp), isdie(false) {
        rect.left = x;
        rect.top = y;
        rect.right = x + width;
        rect.bottom = y + height;
    }

    virtual ~Object() {}

    // 碰撞检测函数
    bool crash(Object& other) {
        return RectDuangRect(rect, other.rect);
    }

    RECT& GetRect() { return rect; }
    unsigned int getHP() const { return HP; }
    void setHP(unsigned int hp) { HP = hp; }
    bool isDead() const { return isdie; }
    void setDead(bool dead) { isdie = dead; }

    // 纯虚函数，由派生类实现移动逻辑
    virtual void Move() = 0;

    // 纯虚函数，由派生类实现显示逻辑
    virtual bool Show() = 0;

    // 纯虚函数，获取敌机类型
    virtual EnemyType getType() const { return NORMAL; }
};

// 胜利界面
void Victory(unsigned long long& kill, int stage) {
    // 保存当前获得的钻石
    currentDiamonds += kill / 150;
    totalDiamonds += currentDiamonds;
    SaveGameData();

    // 创建临时窗口
    HWND hwnd = GetHWnd();
    SetWindowText(hwnd, _T("胜利!"));

    // 初始化绘图
    BeginBatchDraw();
    cleardevice();

    // 显示胜利文字
    settextstyle(80, 0, _T("黑体"));
    settextcolor(GREEN);
    LPCTSTR victoryText = _T("胜利!");
    outtextxy(swidth / 2 - textwidth(victoryText) / 2, sheight / 3, victoryText);

    // 显示关卡信息
    TCHAR stageText[50];
    _stprintf_s(stageText, _T("第%d关 通关"), stage);
    settextstyle(40, 0, _T("黑体"));
    settextcolor(GREEN);
    outtextxy(swidth / 2 - textwidth(stageText) / 2, sheight / 2, stageText);

    // 显示击杀数
    TCHAR killText[50];
    _stprintf_s(killText, _T("得分: %llu"), kill);
    settextstyle(30, 0, _T("黑体"));
    settextcolor(GREEN);
    outtextxy(swidth / 2 - textwidth(killText) / 2, sheight / 2 + 60, killText);

    // 显示钻石获得
    TCHAR diamondText[50];
    _stprintf_s(diamondText, _T("获得钻石: %llu"), currentDiamonds);
    outtextxy(swidth / 2 - textwidth(diamondText) / 2, sheight / 2 + 120, diamondText);

    // 提示按任意键继续
    LPCTSTR continueText = _T("按任意键返回选关界面");
    settextstyle(20, 0, _T("黑体"));
    settextcolor(DARKGRAY);
    outtextxy(swidth / 2 - textwidth(continueText) / 2, sheight * 3 / 4, continueText);

    EndBatchDraw();

    // 等待任意按键
    ExMessage msg;
    flushmessage(EM_KEY | EM_MOUSE);
    while (true) {
        if (peekmessage(&msg, EM_KEY | EM_MOUSE)) {
            if (msg.message == WM_KEYDOWN || msg.message == WM_LBUTTONDOWN) {
                break; // 任意键或鼠标点击退出
            }
        }
        Sleep(20);
    }

    // 重置当前钻石计数
    currentDiamonds = 0;
}

// 背景类
class BK
{
public:
    BK(IMAGE& img) :img(img), y(-sheight) {}

    // 显示背景(滚动效果)
    void Show()
    {
        if (y == 0) { y = -sheight; }
        y += 4;
        putimagePNG(0, y, &img);
    }

private:
    IMAGE& img;
    int y;
};

// 玩家飞机类
class Hero : public Object
{
private:
    IMAGE& img;  // 飞机图片引用

protected:
    bool hasShield = false;
    clock_t shieldEndTime = 0;

    bool hasStrength = false;
    clock_t strengthEndTime = 0;

public:
    Hero(IMAGE& img, StartProp prop)
        : Object(swidth / 2 - img.getwidth() / 2, sheight - img.getheight(),
            img.getwidth(), img.getheight(), SHP), img(img) {
        // 应用开局道具
        switch (prop) {
        case SHIELD:
            ActivateShield(8000); // 8秒护盾
            break;
        case HP_BOOST:
            HP = min(SHP + 1, 5); // 最大5点
            break;
        case STRENGTH:
            ActivateStrength(8000); // 8秒强化
            break;
        }
    }

    // 显示玩家飞机
    void ShowPlayer()
    {
        if (isDead()) return;
        setlinecolor(RED);
        setlinestyle(PS_SOLID, 4);
        putimagePNG(rect.left, rect.top, &img);
        // 绘制生命值条
        line(rect.left, rect.top - 5, rect.left + (img.getwidth() / SHP * HP), rect.top - 5);
        if (CheckShield()) {
            if (shieldEndTime - clock() <= 2000) { // 最后2秒闪烁
                if (clock() % 250 < 100) { // 每250毫秒闪烁一次
                    setlinecolor(RGB(0, 255, 255));
                    setlinestyle(PS_SOLID, 3);
                    circle(rect.left + img.getwidth() / 2, rect.top + img.getheight() / 2, img.getwidth());
                }
            }
            else {
                setlinecolor(RGB(0, 255, 255));
                setlinestyle(PS_SOLID, 3);
                circle(rect.left + img.getwidth() / 2, rect.top + img.getheight() / 2, img.getwidth());
            }
        }
    }

    // 控制玩家飞机移动
    void Control()
    {
        if (isDead()) return;
        int speed = 30; // 移动速度

        // 检测按键的状态
        if (GetAsyncKeyState(VK_LEFT) & 0x8000)  // 按下左箭头键
        {
            rect.left -= speed;
        }
        if (GetAsyncKeyState(VK_RIGHT) & 0x8000)  // 按下右箭头键
        {
            rect.left += speed;
        }
        if (GetAsyncKeyState(VK_UP) & 0x8000)  // 按下上箭头键
        {
            rect.top -= speed;
        }
        if (GetAsyncKeyState(VK_DOWN) & 0x8000)  // 按下下箭头键
        {
            rect.top += speed;
        }

        // 更新 rect.right 和 rect.bottom
        rect.right = rect.left + img.getwidth();
        rect.bottom = rect.top + img.getheight();

        // 边界限制
        if (rect.left < 0) rect.left = 0;
        if (rect.top < 0) rect.top = 0;
        if (rect.right > swidth) rect.left = swidth - img.getwidth();
        if (rect.bottom > sheight) rect.top = sheight - img.getheight();

        // 再次更新 rect.right 和 rect.bottom
        rect.right = rect.left + img.getwidth();
        rect.bottom = rect.top + img.getheight();

    }

    // 受伤处理
    bool hurt()
    {
        if (CheckShield()) {
            return true;
        }
        if (HP > 0) {
            HP--;
        }
        if (HP <= 0) {
            setDead(true);  // 添加：设置死亡标志
            return false;
        }
        return true;
    }

    // 实现基类纯虚函数
    void Move() override {
        // 玩家飞机由鼠标控制，Move()不做处理
    }

    bool Show() override {
        ShowPlayer();
        return !isDead();
    }
    //护盾功能
    void ActivateShield(int duration) {
        shieldEndTime = clock() + duration;
        hasShield = true;
    }
    bool CheckShield() {
        if (hasShield && clock() > shieldEndTime) {
            hasShield = false;
        }
        return hasShield;
    }

    //血包功能
    void AddHP(int amount) {
        HP = min(HP + amount, SHP); // 不超过最大生命值
    }

    //强化子弹功能
    void ActivateStrength(int duration) {
        strengthEndTime = clock() + duration;
        hasStrength = true;
    }
    bool CheckStrength() {
        if (hasShield && clock() > strengthEndTime) {
            hasStrength = false;
        }
        return hasStrength;
    }

    // 炸弹功能
    void Boom() {
        HP = 0;
        setDead(true);
    }
};

class Hero2 : public Hero
{
private:IMAGE& img;
public:
    Hero2(IMAGE& x, StartProp prop) : Hero(x, prop), img(x) {}
    void Control()
    {
        if (isDead()) return;
        int speed = 30; // 移动速度

        // 检测按键的状态
        if (GetAsyncKeyState('A') & 0x8000)  // 按下左箭头键
        {
            rect.left -= speed;
        }
        if (GetAsyncKeyState('D') & 0x8000)  // 按下右箭头键
        {
            rect.left += speed;
        }
        if (GetAsyncKeyState('W') & 0x8000)  // 按下上箭头键
        {
            rect.top -= speed;
        }
        if (GetAsyncKeyState('S') & 0x8000)  // 按下下箭头键
        {
            rect.top += speed;
        }

        // 更新 rect.right 和 rect.bottom
        rect.right = rect.left + img.getwidth();
        rect.bottom = rect.top + img.getheight();

        // 边界限制
        if (rect.left < 0) rect.left = 0;
        if (rect.top < 0) rect.top = 0;
        if (rect.right > swidth) rect.left = swidth - img.getwidth();
        if (rect.bottom > sheight) rect.top = sheight - img.getheight();

        // 再次更新 rect.right 和 rect.bottom
        rect.right = rect.left + img.getwidth();
        rect.bottom = rect.top + img.getheight();

    }
};
// 敌机基类
class Enemy : public Object
{
protected:
    IMAGE& img;         // 敌机图片引用
    IMAGE selfboom[3];  // 爆炸效果图片
    int boomsum;        // 爆炸动画帧数
    EnemyType type;  // 敌机类型

public:
    static int stage;
    Enemy(IMAGE& img, int x, IMAGE*& boom, unsigned int hp, EnemyType t)
        : Object(x, -img.getheight(), img.getwidth(), img.getheight(), hp),
        img(img), boomsum(0), type(t)
    {
        // 加载爆炸效果图片
        selfboom[0] = boom[0];
        selfboom[1] = boom[1];
        selfboom[2] = boom[2];
    }

    // 显示敌机
    virtual bool Show()
    {
        if (isdie)
        {
            if (boomsum == 3)
            {
                return false;
            }
            // 播放爆炸动画
            putimagePNG(rect.left, rect.top, selfboom + boomsum);
            boomsum++;
            return true;
        }

        if (rect.top >= sheight)
        {
            return false;
        }

        // 绘制敌机
        putimagePNG(rect.left, rect.top, &img);

        // 为高血飞机和Boss绘制生命值条
        if (type == TANK || type == BOSS) {
            setlinecolor(YELLOW);
            setlinestyle(PS_SOLID, 2);
            int maxWidth = img.getwidth();
            int currentWidth = (maxWidth * HP) / getMaxHP(type, stage);
            line(rect.left, rect.top - 5, rect.left + currentWidth, rect.top - 5);
        }

        return true;
    }

    void Isdie() { isdie = true; }

    // 获取敌机类型
    EnemyType getType() const override { return type; }

    void setstage(int stag) { stage = stag; }
    // 根据敌机类型获取最大生命值
    static unsigned int getMaxHP(EnemyType t, int x) {
        switch (t) {
        case NORMAL: return 1;
        case SUICIDE: return 1;
        case TANK: return 3;
        case FAST: return 1;
        case BOSS:
            switch (x) {
            case 1:return 4;
            case 2:return 7;
            case 3:return 11;
            }

        default: return 1;
        }
    }
};

// 普通敌机
class NormalEnemy : public Enemy
{
public:
    NormalEnemy(IMAGE& img, int x, IMAGE*& boom)
        : Enemy(img, x, boom, 1, NORMAL) {
    }

    // 实现基类纯虚函数
    void Move() override {
        // 普通敌机向下移动
        rect.top += 5;
        rect.bottom += 5;
    }

    bool Show() override {
        Move();
        return Enemy::Show();
    }
};

// 自爆敌机
class SuicideEnemy : public Enemy
{
private:
    Hero* hero;  // 玩家飞机指针
    bool locked; // 是否已锁定目标

public:
    SuicideEnemy(IMAGE& img, int x, IMAGE*& boom, Hero* h)
        : Enemy(img, x, boom, 1, SUICIDE), hero(h), locked(false) {
    }

    // 实现基类纯虚函数
    void Move() override {
        if (!locked && rand() % 100 < 30) { // 30%概率锁定玩家
            locked = true;
        }

        if (locked) {
            // 向玩家位置移动
            int targetX = hero->GetRect().left + (hero->GetRect().right - hero->GetRect().left) / 2;
            int enemyX = rect.left + (rect.right - rect.left) / 2;

            if (abs(targetX - enemyX) > 5) {
                rect.left += (targetX > enemyX) ? 3 : -3;
                rect.right += (targetX > enemyX) ? 3 : -3;
            }
        }

        // 向下移动
        rect.top += 5;
        rect.bottom += 5;
    }

    bool Show() override {
        Move();
        return Enemy::Show();
    }
};

// 高血敌机
class TankEnemy : public Enemy
{
public:
    TankEnemy(IMAGE& img, int x, IMAGE*& boom)
        : Enemy(img, x, boom, 3, TANK) {
    }

    // 实现基类纯虚函数
    void Move() override {
        // 高血敌机移动较慢
        rect.top += 3;
        rect.bottom += 3;
    }

    bool Show() override {
        Move();
        return Enemy::Show();
    }
};

// 高速敌机
class FastEnemy : public Enemy
{
public:
    FastEnemy(IMAGE& img, int x, IMAGE*& boom)
        : Enemy(img, x, boom, 1, FAST) {
    }

    // 实现基类纯虚函数
    void Move() override {
        // 高速敌机移动较快
        rect.top += 7;
        rect.bottom += 7;
    }

    bool Show() override {
        Move();
        return Enemy::Show();
    }
};

// Boss敌机
class BossEnemy : public Enemy
{
private:
    int direction;  // 移动方向：-1左，1右
    int shootTimer; // 射击计时器
    IMAGE& bossImg; // Boss专用图片
    int originalY;  // Boss初始Y坐标，用于保持在屏幕上方

public:
    BossEnemy(IMAGE& img, IMAGE*& boom, IMAGE& bossImg)
        : Enemy(img, swidth / 2 - img.getwidth() / 2, boom, getMaxHP(BOSS, stage), BOSS),
        direction(1), shootTimer(0), bossImg(bossImg)
    {
        // 设置Boss初始位置在屏幕顶部
        originalY = 50;
        rect.top = originalY;
        rect.bottom = originalY + bossImg.getheight();
    }

    // 实现基类纯虚函数
    void Move() override {
        // Boss保持在屏幕顶部，只左右移动
        rect.left += direction * 2;
        rect.right += direction * 2;

        // 保持Y坐标不变
        rect.top = originalY;
        rect.bottom = originalY + bossImg.getheight();

        // 碰到边界改变方向
        if (rect.left <= 0 || rect.right >= swidth) {
            direction = -direction;
        }
    }

    bool Show() override {
        Move();

        if (isdie) {
            return Enemy::Show();
        }

        // 绘制Boss
        putimagePNG(rect.left, rect.top, &bossImg);

        // 绘制Boss生命值条
        setlinecolor(RED);
        setlinestyle(PS_SOLID, 3);
        int maxWidth = bossImg.getwidth();
        int currentWidth = (maxWidth * HP) / getMaxHP(type, stage);
        line(rect.left, rect.top - 10, rect.left + currentWidth, rect.top - 10);

        // 显示Boss血量数字
        TCHAR hpText[20];
        _stprintf_s(hpText, _T("Boss: %d/%d"), HP, getMaxHP(type, stage));
        settextstyle(16, 0, _T("黑体"));
        settextcolor(RED);
        outtextxy(rect.left + 10, rect.top - 30, hpText);

        // 更新射击计时器
        shootTimer++;

        return true;
    }

    // Boss专用射击方法
    bool canShoot() const {
        return shootTimer >= 40; // 每40帧射击一次
    }

    void resetShootTimer() {
        shootTimer = 0;
    }
};

// 子弹基类
class Bullet : public Object
{
public:
    Bullet(IMAGE& img, RECT pr)
        : Object(pr.left + (pr.right - pr.left) / 2 - img.getwidth() / 2,
            pr.top - img.getheight(), img.getwidth(), img.getheight(), 1),
        img(img) {
    }

    // 显示子弹(向上移动)
    bool Show() override
    {
        if (rect.bottom <= 0)
        {
            return false;
        }
        // 子弹向上移动
        rect.top -= 3;
        rect.bottom -= 3;
        putimagePNG(rect.left, rect.top, &img);

        return true;
    }

    // 实现基类纯虚函数
    void Move() override {
        // 子弹移动逻辑已在Show()中实现
    }

protected:
    IMAGE& img;
};

// 敌机子弹类
class EBullet : public Bullet
{
public:
    EBullet(IMAGE& img, RECT pr)
        : Bullet(img, pr)
    {
        // 调整敌机子弹初始位置
        rect.top = pr.bottom;
        rect.bottom = rect.top + img.getheight();
    }

    // 显示子弹(向下移动)
    bool Show() override
    {
        if (rect.top >= sheight)
        {
            return false;
        }
        // 子弹向下移动
        rect.top += 5;
        rect.bottom += 5;
        putimagePNG(rect.left, rect.top, &img);

        return true;
    }
};

// 添加敌机到敌机列表
// 添加敌机到敌机列表
bool AddEnemy(vector<Enemy*>& es, IMAGE& enemyimg, IMAGE* boom, Hero* hero, IMAGE& bossImg, int stage = 1, unsigned long long kill = 0)
{
    // 检查是否需要生成Boss
    bool isBossTime = false;

    // 只有在分数达到要求且当前没有Boss时才生成Boss
    if ((stage == 1 && kill >= 100) || (stage == 2 && kill >= 400) || (stage == 3 && kill >= 900)) { // 限制stage <= 2
        bool hasBoss = false;
        for (auto e : es) {
            if (e != nullptr && e->getType() == BOSS) {
                hasBoss = true;
                break;
            }
        }
        isBossTime = !hasBoss;
    }

    if (isBossTime) {
        // 清除所有现有敌机
        for (auto e : es) {
            delete e;
        }
        es.clear();

        // 居中生成Boss
        Enemy* e = new BossEnemy(enemyimg, boom, bossImg);

        if (!e) return false;
        es.push_back(e);
        return true;
    }

    // 普通敌机生成逻辑
    EnemyType type;
    int randType = rand() % 100;

    switch (stage) {
    case 1: // 第一关：普通和自爆敌人
        if (randType < 30) {
            type = SUICIDE;
        }
        else {
            type = NORMAL;
        }
        break;
    case 2: // 第二关：普通、自爆、高血、高速敌人
        if (randType < 25) {
            type = SUICIDE;
        }
        else if (randType < 45) {
            type = TANK;
        }
        else if (randType < 65) {
            type = FAST;
        }
        else {
            type = NORMAL;
        }
        break;
    case 3:
        if (randType < 33) {
            type = SUICIDE;
        }
        else if (randType < 66) {
            type = TANK;
        }
        else {
            type = FAST;
        }
    default: // 默认情况同第一关
        if (randType < 30) {
            type = SUICIDE;
        }
        else {
            type = NORMAL;
        }
    }

    // 普通敌机随机位置生成
    int x = abs(rand()) % (swidth - enemyimg.getwidth());
    Enemy* e = nullptr;

    // 根据类型创建敌机
    switch (type) {
    case NORMAL:
        e = new NormalEnemy(enemyimg, x, boom);
        break;
    case SUICIDE:
        e = new SuicideEnemy(enemyimg, x, boom, hero);
        break;
    case TANK:
        e = new TankEnemy(enemyimg, x, boom);
        break;
    case FAST:
        e = new FastEnemy(enemyimg, x, boom);
        break;
    case BOSS:
        // 这部分理论上不会执行，因为Boss已单独处理
        break;
    }

    if (!e) return false;

    // 检查是否与其他敌机重叠
    bool overlaps = false;
    for (auto& i : es)
    {
        if (i != nullptr && RectDuangRect(i->GetRect(), e->GetRect()))
        {
            overlaps = true;
            break;
        }
    }

    if (overlaps) {
        delete e;
        return false;
    }

    es.push_back(e);
    return true;
}
// 游戏主循环
// 游戏主循环
int Enemy::stage = 1;
bool Play(StartProp selectedProp)
{

    setbkcolor(WHITE);
    cleardevice();
    bool is_play = true;
    bool is_play2 = (numberOfPlayers == 2);
    clock_t startTime = clock();
    int stage = ModeNumber;
    Enemy::stage = stage;  // 关卡数
    unsigned long long kill = 0; // 击杀计数
    unsigned long long kill2 = 0;
    bool inBossFight = false;    // 是否处于Boss战
    int bossTimer = 0;           // Boss战计时器(用于提示)
    int maxStages = 3;           // 最大关卡数

    // 加载游戏资源图片
    IMAGE heroimg, enemyimg, bkimg, bossImg;
    IMAGE bulletimg[2];//敌我子弹
    IMAGE eboom[3];
    IMAGE propimg[5];//道具

    loadimage(&heroimg, _T("../images/me1.png"));   // 玩家飞机
    loadimage(&enemyimg, _T("../images/enemy1.png")); // 敌机
    loadimage(&bkimg, _T("../images/bk2.png"), swidth, sheight * 2); // 背景
    loadimage(&bossImg, _T("../images/enemy2.png"));   // Boss敌机

    // 加载爆炸效果图片
    loadimage(&eboom[0], _T("../images/enemy1_down2.png"));
    loadimage(&eboom[1], _T("../images/enemy1_down3.png"));
    loadimage(&eboom[2], _T("../images/enemy1_down4.png"));

    // 区分敌我子弹
    loadimage(&bulletimg[0], _T("../images/bullet1.png"));
    loadimage(&bulletimg[1], _T("../images/bullet2.png"));

    // 道具图片
    loadimage(&propimg[0], _T("../images/heart.png"));
    loadimage(&propimg[1], _T("../images/shield.png"));
    loadimage(&propimg[2], _T("../images/strength.png"));
    loadimage(&propimg[3], _T("../images/boom.png"));;
    loadimage(&propimg[4], _T("../images/diamond.png"));;
    // 初始化游戏对象
    BK bk = BK(bkimg);
    Hero hp = Hero(heroimg, selectedProp);  // 玩家飞机
    Hero2 hp2 = Hero2(heroimg, selectedProp);//第二架飞机

    vector<Prop*> ps; // 道具
    vector<Enemy*> es;  // 敌机列表
    vector<Bullet*> bs; // 玩家子弹列表
    vector<Bullet*> bs2; // 玩家子弹列表
    vector<EBullet*> ebs; // 敌机子弹列表
    int bsing = 0;       // 子弹发射计数器

    clock_t hurtlast = clock();  // 上次受伤时间
    clock_t hurtlast2 = clock();  // 上次受伤时间
    clock_t hurtlast3 = clock();
    // 初始化随机数种子
    srand((unsigned)time(NULL));

    // 初始添加10个敌机
    for (int i = 0; i < 10; i++)
    {
        AddEnemy(es, enemyimg, eboom, &hp, bossImg, stage, kill);
    }

    // 游戏主循环
    while (is_play || is_play2)
    {
        if (!is_play2) {
            hp2.Boom();
        }
        bsing++;
        // 每隔10帧发射玩家子弹
        if (bsing % 10 == 0)
        {
            if (is_play) {
                // 强化子弹功能
                if (hp.CheckStrength()) {
                    // 获取飞机矩形
                    RECT hero1Rect = hp.GetRect();

                    // 左侧子弹（向右偏移）
                    RECT leftPos1 = hero1Rect;
                    leftPos1.left += hero1Rect.right - hero1Rect.left - 25;
                    leftPos1.right = leftPos1.left + bulletimg[0].getwidth();
                    bs.push_back(new Bullet(bulletimg[0], leftPos1));

                    // 右侧子弹（向左偏移）
                    RECT rightPos1 = hero1Rect;
                    rightPos1.left += 25;
                    rightPos1.right = rightPos1.left + bulletimg[0].getwidth();
                    bs.push_back(new Bullet(bulletimg[0], rightPos1));
                }
                else {
                    bs.push_back(new Bullet(bulletimg[0], hp.GetRect()));
                }
            }
            if (is_play2) {
                // 强化子弹功能
                if (hp2.CheckStrength()) {
                    // 获取飞机矩形
                    RECT hero2Rect = hp2.GetRect();

                    // 左侧子弹（向右偏移）
                    RECT leftPos2 = hero2Rect;
                    leftPos2.left += hero2Rect.right - hero2Rect.left - 25;
                    leftPos2.right = leftPos2.left + bulletimg[0].getwidth();
                    bs2.push_back(new Bullet(bulletimg[0], leftPos2));

                    // 右侧子弹（向左偏移）
                    RECT rightPos2 = hero2Rect;
                    rightPos2.left += 25;
                    rightPos2.right = rightPos2.left + bulletimg[0].getwidth();
                    bs2.push_back(new Bullet(bulletimg[0], rightPos2));
                }
                else {
                    bs2.push_back(new Bullet(bulletimg[0], hp2.GetRect()));
                }
            }
        }
        // 每隔60帧敌机发射子弹
        if (bsing == 20)
        {
            bsing = 0;
            for (auto it = es.begin(); it != es.end(); )
            {
                Enemy* e = *it;
                if (e == nullptr) {
                    it = es.erase(it);
                    continue;
                }

                // Boss有专用射击逻辑
                if (e->getType() == BOSS) {
                    BossEnemy* boss = dynamic_cast<BossEnemy*>(e);
                    if (boss->canShoot()) {
                        // Boss发射多颗子弹
                        RECT r = e->GetRect();
                        int centerX = r.left + (r.right - r.left) / 2;
                        int centerY = r.bottom;

                        // 中间子弹
                        ebs.push_back(new EBullet(bulletimg[1], r));

                        // 两侧子弹
                        for (int j = -1; j <= 1; j += 2) {
                            RECT sideR = r;
                            sideR.left = centerX - 10 + j * 20;
                            sideR.right = sideR.left + 20;
                            ebs.push_back(new EBullet(bulletimg[1], sideR));
                        }

                        boss->resetShootTimer();
                    }
                }
                else {
                    ebs.push_back(new EBullet(bulletimg[1], e->GetRect()));
                }
                ++it;
            }
        }

        BeginBatchDraw();

        bk.Show();
        Sleep(2);
        flushmessage();
        Sleep(2);
        hp.Control();
        hp2.Control();

        // 显示分数、关卡和状态
        TCHAR statusText[100];
        if (inBossFight) {
            _stprintf_s(statusText, _T("分数: %llu  关卡: %d  Boss战!"), kill, stage);
        }
        else {
            _stprintf_s(statusText, _T("分数: %llu  关卡: %d  目标: %d分"), kill, stage, 100 * stage * stage);
        }
        settextstyle(20, 0, _T("黑体"));
        settextcolor(BLACK);
        outtextxy(10, 10, statusText);

        // 显示钻石UI（右上角）
        TCHAR diamondInfo[64];
        _stprintf_s(diamondInfo, _T("钻石: %llu"), totalDiamonds);
        settextstyle(20, 0, _T("黑体"));
        settextcolor(BLACK);
        setbkmode(OPAQUE);
        outtextxy(swidth - 200, 10, diamondInfo);

        // 显示Boss即将到来的提示
        if (!inBossFight && ((stage == 1 && kill >= 100) || (stage == 2 && kill >= 400) || (stage == 3 && kill >= 900)) && bossTimer < 80) {
            TCHAR bossText[50];
            _stprintf_s(bossText, _T("第%d关Boss即将到来!"), stage);
            settextstyle(30, 0, _T("黑体"));
            settextcolor(RED);
            outtextxy(swidth / 2 - textwidth(bossText) / 2, sheight / 2, bossText);
            bossTimer++;
        }
        else if (!inBossFight && ((stage == 1 && kill >= 100) || (stage == 2 && kill >= 400) || (stage == 3 && kill >= 900)) && bossTimer >= 80) {
            // 达到分数且提示时间结束，触发Boss战
            inBossFight = true;
            bossTimer = 0;
            // 清空敌机和子弹
            for (auto e : es) {
                delete e;
            }
            es.clear();

            for (auto b : bs) {
                delete b;
            }
            bs.clear();
            for (auto b : bs2) {
                delete b;
            }
            bs2.clear();

            for (auto eb : ebs) {
                delete eb;
            }
            ebs.clear();

            // 生成Boss
            AddEnemy(es, enemyimg, eboom, &hp, bossImg, stage, kill);
        }

        // 暂停功能(按空格键)
        if (_kbhit())
        {
            char v = _getch();
            if (v == 0x20)  // 空格键
            {
                Sleep(500);
                while (true)
                {
                    if (_kbhit())
                    {
                        v = _getch();
                        if (v == 0x20)
                        {
                            break;
                        }
                    }
                    Sleep(16);
                }
            }
        }
        hp.Show();
        hp2.Show();

        // 显示玩家子弹
        for (auto it = bs.begin(); it != bs.end(); )
        {
            Bullet* b = *it;
            if (b == nullptr || !b->Show())
            {
                delete b;
                it = bs.erase(it);
            }
            else
            {
                ++it;
            }
        }
        for (auto it2 = bs2.begin(); it2 != bs2.end(); )
        {
            Bullet* c = *it2;
            if (c == nullptr || !c->Show())
            {
                delete c;
                it2 = bs2.erase(it2);
            }
            else
            {
                ++it2;
            }
        }

        // 显示敌机子弹并检测碰撞
        for (auto it = ebs.begin(); it != ebs.end(); )
        {
            EBullet* eb = *it;
            if (eb == nullptr || !eb->Show())
            {
                delete eb;
                it = ebs.erase(it);
            }
            else
            {
                // 检测是否击中玩家飞机
                if (RectDuangRect(eb->GetRect(), hp.GetRect()))
                {
                    if (clock() - hurtlast >= hurttime)
                    {
                        is_play = hp.hurt();
                        hurtlast = clock();
                    }
                    delete eb;
                    it = ebs.erase(it);
                    continue;
                }
                if (RectDuangRect(eb->GetRect(), hp2.GetRect()) && eb != nullptr)
                {
                    if (clock() - hurtlast >= hurttime)
                    {
                        is_play2 = hp2.hurt();
                        hurtlast = clock();
                    }
                    delete eb;
                    it = ebs.erase(it);
                    continue;
                }
                ++it;
            }
        }

        // 处理敌机
        bool bossAlive = false;
        for (auto it = es.begin(); it != es.end(); )
        {
            Enemy* e = *it;
            if (e == nullptr) {
                it = es.erase(it);
                continue;
            }

            // 检测敌机与玩家飞机碰撞
            if (e->getType() != BOSS && e->crash(hp)) // Boss不与玩家直接碰撞
            {
                if (clock() - hurtlast >= hurttime)
                {
                    is_play = hp.hurt();
                    hurtlast = clock();
                }
                e->Isdie(); // 敌机自爆
            }
            if (e->getType() != BOSS && e->crash(hp2) && !e->isDead()) // Boss不与玩家直接碰撞
            {
                if (clock() - hurtlast2 >= hurttime)
                {
                    is_play2 = hp2.hurt();
                    hurtlast2 = clock();
                }
                e->Isdie(); // 敌机自爆
            }

            // 检测敌机与玩家子弹碰撞
 // 原始代码...
// 检测敌机与玩家子弹碰撞
            for (auto bit = bs.begin(); bit != bs.end(); )
            {
                Bullet* b = *bit;
                if (b == nullptr) {
                    bit = bs.erase(bit);
                    continue;
                }

                if (((e->getType() != BOSS) && (e->crash(*b))) || ((e->getType() == BOSS) && (e->crash(*b)) && (clock() - hurtlast3 >= 2000)))
                {


                    // 确保只在子弹与敌机重叠时减少血量
                    e->setHP(e->getHP() - 1);
                    if (e->getType() == BOSS) { hurtlast3 = clock(); }

                    if (e->getHP() <= 0) {
                        e->Isdie();  // 敌机死亡

                        // 根据敌机类型增加不同分数
                        switch (e->getType()) {
                        case NORMAL:
                            kill += 4;
                            break;
                        case SUICIDE:
                        case FAST:
                            kill += 10;
                            break;
                        case TANK:
                            kill += 30;
                            break;
                        case BOSS:
                            kill += 100;
                            inBossFight = false;
                            bossTimer = 0;

                            Victory(kill, stage);

                            // 解锁下一关
                            if (stage == unlockedLevel && unlockedLevel < MAX_LEVELS) {
                                unlockedLevel++;
                            }
                            SaveGameData();

                            return true;
                            // 清空敌机和子弹
                            for (auto e : es) delete e;
                            es.clear();
                            for (auto b : bs) delete b;
                            bs.clear();
                            for (auto eb : ebs) delete eb;
                            ebs.clear();

                            // 初始化新关卡的敌机
                            for (int i = 0; i < 5; i++) {
                                AddEnemy(es, enemyimg, eboom, &hp, bossImg, stage, kill);
                            }


                            break;
                        }
                    }

                    // 只删除击中的子弹，而不是全部子弹
                    delete b;
                    bit = bs.erase(bit);
                    break; // 跳出这个子弹循环，因为敌机已经被击中
                }
                else
                {
                    ++bit;
                }
            }

            // 同样修改Hero2的子弹检测
            for (auto bit = bs2.begin(); bit != bs2.end(); )
            {
                Bullet* b = *bit;
                if (b == nullptr) {
                    bit = bs2.erase(bit);
                    continue;
                }

                if (((e->getType() != BOSS) && (e->crash(*b))) || ((e->getType() == BOSS) && (e->crash(*b)) && (clock() - hurtlast3 >= 2000)))
                {
                    // 确保只在子弹与敌机重叠时减少血量
                    e->setHP(e->getHP() - 1);
                    if (e->getType() == BOSS) { hurtlast3 = clock(); }

                    if (e->getHP() <= 0) {
                        e->Isdie();  // 敌机死亡

                        // 根据敌机类型增加不同分数
                        switch (e->getType()) {
                        case NORMAL:
                        case SUICIDE:
                        case FAST:
                            kill += 10;
                            break;
                        case TANK:
                            kill += 30;
                            break;
                        case BOSS:
                            kill += 100;
                            inBossFight = false;
                            bossTimer = 0;

                            Victory(kill, stage);

                            // 解锁下一关
                            if (stage == unlockedLevel && unlockedLevel < MAX_LEVELS) {
                                unlockedLevel++;
                            }

                            SaveGameData();

                            return true;

                            // 清空敌机和子弹
                            for (auto e : es) delete e;
                            es.clear();
                            for (auto b : bs2) delete b;
                            bs2.clear();
                            for (auto eb : ebs) delete eb;
                            ebs.clear();

                            // 初始化新关卡的敌机
                            for (int i = 0; i < 5; i++) {
                                AddEnemy(es, enemyimg, eboom, &hp2, bossImg, stage, kill);
                            }

                            // 如果通关则结束游戏

                        }
                    }

                    // 只删除击中的子弹，而不是全部子弹
                    delete b;
                    bit = bs2.erase(bit);
                    break; // 跳出这个子弹循环，因为敌机已经被击中
                }
                else
                {
                    ++bit;
                }
            }
            // 原始代码...

            // Boss发射子弹
            if (e->getType() == BOSS && !e->isDead()) {
                BossEnemy* boss = dynamic_cast<BossEnemy*>(e);
                if (boss->canShoot()) {
                    // Boss发射多颗子弹
                    RECT r = e->GetRect();
                    int centerX = r.left + (r.right - r.left) / 2;
                    int centerY = r.bottom;

                    // 中间子弹
                    ebs.push_back(new EBullet(bulletimg[1], r));

                    // 两侧子弹
                    for (int j = -1; j <= 1; j += 2) {
                        RECT sideR = r;
                        sideR.left = centerX - 10 + j * 20;
                        sideR.right = sideR.left + 20;
                        ebs.push_back(new EBullet(bulletimg[1], sideR));
                    }

                    boss->resetShootTimer();
                }
                bossAlive = true;
            }

            // 显示敌机
            if (!e->Show())
            {
                delete e;
                it = es.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // 如果Boss被击败，更新状态
        if (inBossFight && !bossAlive) {
            inBossFight = false;
        }

        // 确保敌机数量始终保持在5架(非Boss战时)
        if (!inBossFight && es.size() < 10) {
            AddEnemy(es, enemyimg, eboom, &hp, bossImg, stage, kill);
        }
        propSpawnTimer++;
        if (propSpawnTimer >= PROP_SPAWN_INTERVAL) {
            int x = rand() % (swidth - propimg[0].getwidth());
            int type = rand() % 4; // 随机生成
            ps.push_back(new Prop(propimg[type], type, x, -propimg[type].getheight()));
            propSpawnTimer = 0;
        }
        if (diamondSpawnTimer <= 0) {
            // 生成钻石（type=4）
            int x = rand() % (swidth - propimg[4].getwidth());
            ps.push_back(new Prop(propimg[4], 4, x, -propimg[4].getheight()));
            // 重置为随机间隔
            diamondSpawnTimer = rand() % (DIAMOND_MAX_DELAY - DIAMOND_MIN_DELAY) + DIAMOND_MIN_DELAY;
        }
        else {
            diamondSpawnTimer--;
        }
        // 更新并检测道具
        auto propIt = ps.begin();
        while (propIt != ps.end()) {
            if (!(*propIt)->Update()) { // 道具移出屏幕
                delete* propIt;
                propIt = ps.erase(propIt);
            }
            else {
                (*propIt)->Show(); // 渲染道具

                bool collected = false;

                // 检查道具和玩家的碰撞
                if (RectDuangRect((*propIt)->GetRect(), hp.GetRect())) {
                    switch ((*propIt)->GetType()) {
                    case 0: // 回血
                        hp.AddHP(1);
                        break;
                    case 1: // 护盾
                        hp.ActivateShield(3000); // 激活护盾3秒
                        break;
                    case 2: // 强化
                        hp.ActivateStrength(3000);
                        break;
                    case 3: // 爆炸
                        hp.Boom();
                        is_play = hp.hurt();
                        break;
                    case 4: // 钻石
                        currentDiamonds++;   // 当前钻石+1
                        totalDiamonds++;     // 总钻石+1
                        break;
                    }
                    collected = true;
                }
                else if (RectDuangRect((*propIt)->GetRect(), hp2.GetRect())) {
                    switch ((*propIt)->GetType()) {
                    case 0: // 回血
                        hp2.AddHP(1);
                        break;
                    case 1: // 护盾
                        hp2.ActivateShield(3000); // 激活护盾3秒
                        break;
                    case 2: // 强化
                        hp2.ActivateStrength(3000);
                        break;
                    case 3: // 爆炸
                        hp2.Boom();
                        is_play = hp2.hurt();
                        break;
                    case 4: // 钻石
                        currentDiamonds++;   // 当前钻石+1
                        totalDiamonds++;     // 总钻石+1
                        break;
                    }
                    collected = true;
                }

                if (collected) {
                    delete* propIt;
                    propIt = ps.erase(propIt);
                }
                else {
                    ++propIt;
                }
            }
        }
        EndBatchDraw();
    }

    Over(kill, startTime);

    // 清理所有剩余对象
    for (auto e : es) delete e;
    es.clear();

    for (auto b : bs) delete b;
    bs.clear();
    for (auto b : bs2) delete b;
    bs2.clear();

    for (auto eb : ebs) delete eb;
    ebs.clear();

    return true;
}


// 主函数
int main()
{
    // 初始化EasyX图形窗口
    initgraph(swidth, sheight, EW_NOMINIMIZE | EW_SHOWCONSOLE);//参数 EW_NOMINIMIZE 禁止窗口最小化，EW_SHOWCONSOLE 显示控制台（用于调试输出）
    LoadGameData();

    bool is_live = true;

    // 游戏循环
    while (is_live)//Welcome() 显示游戏欢迎界面，等待用户操作（如按任意键开始）
    {
        Welcome1();
        Welcome2();
        if (ModeNumber != 0) {
            Welcome3();
        }
        StartProp selectedProp = ShowPrepareInterface();
        bool gameResult = Play(selectedProp);
        currentDiamonds = 0;
    }

    closegraph();
    return 0;
}