#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#define WIDTH 32
#define HEIGHT 22

// 窗口
typedef struct border {
    int startx;
    int starty;
    int width;
    int height;
} Border;

//方块类型
typedef enum blockType {
    O, S1, S2, Z1, Z2, L1, L2, L3, L4, J1, J2, J3, J4, I1, I2, T1, T2, T3, T4
} BType;

//坐标
typedef struct coordinate {
    int x;
    int y;
} Coordinate;

//掉落的方块
typedef struct bolck {
    Coordinate b[4];
    BType type;
} Block;


//创建窗口和边框
WINDOW *createWin(Border *border);

//初始化窗口显示
void initWin(WINDOW *win, Block *block, Border border, int pile[][WIDTH]);

//处理键盘控制
int dealControl(WINDOW *win, Block *block, int pile[][WIDTH], int *speed, Border border);

int checkLanding(WINDOW *win, Block *block, int pile[][WIDTH], Border border);

int checkCrash(int pile[][WIDTH], Border border);

int checkEliminate(WINDOW *win, int pile[][WIDTH], Border border);

void createBlock(WINDOW *win, Block *block, Border border);

int spin(WINDOW *win,Block *block, int pile[][WIDTH], Border border);

void refreshBlock(WINDOW *win, Block *block);

void leftMove(WINDOW *win, Block *block, int pile[][WIDTH], Border border);

void rightMove(WINDOW *win, Block *block, int pile[][WIDTH], Border border);

void down(WINDOW *win, Block *block, int pile[][WIDTH], Border border);

//销毁窗口
void destroyWin(WINDOW *win);

int main() {

    srand((unsigned)time(NULL));

    //ncurses初始化
    initscr();              // 初始化并进入 curses 模式
    cbreak();               // 行缓冲禁止，传递所有控制信息
    noecho();               // 禁止回显
    keypad(stdscr, TRUE);   // 程序需要使用键盘
    curs_set(0);            //隐藏光标

    //程序数据初始化
    int speed = 500000;
    Border border;
    Block block;
    // 游戏区域映射为数组
    int pile[HEIGHT][WIDTH] = {0};
    //边界赋其他值来区分
    for (int i = 0; i < HEIGHT; ++i) {
        pile[i][0]=2;
        pile[i][WIDTH-1]=2;
    }
    for (int j = 1; j < WIDTH-1; ++j) {
        pile[0][j]=2;
    }
    for (int j = 1; j < WIDTH-1; ++j) {
        pile[HEIGHT-1][j]=2;
    }


    WINDOW *win = createWin(&border);

    initWin(win, &block, border, pile);

    timeout(0);
    while (1) {

        while (dealControl(win, &block, pile, &speed, border)) ;   //按键

        if (checkLanding(win, &block, pile, border)) {
            checkEliminate(win, pile, border);
            if (checkCrash(pile, border)) {
                break;
            }
            createBlock(win, &block, border);
        } else {
            refreshBlock(win, &block);
        }

        usleep(speed);
    }


    destroyWin(win);
    endwin();

    return 0;
}


//创建窗口和边框
WINDOW *createWin(Border *border) {
    //需要用户将命令行窗口尺寸手动调节超过32*22
    border->height = HEIGHT;                        //窗口高度
    border->width = WIDTH;                          //窗口宽度
    border->starty = (LINES - border->height) / 2;    // 计算窗口中心位置的行数
    border->startx = (COLS - border->width) / 2;      // 计算窗口中心位置的列数

    //显示游戏名称和操作提示
    char *name = "Tetris 1.0";
    mvprintw(0, (COLS - strlen(name)) / 2, name);
    char *message = "Press any key continue ...";
    mvprintw(LINES - 1, (COLS - strlen(message)) / 2, message);
    refresh();

    //创建窗口
    WINDOW *local_win;
    local_win = newwin(border->height, border->width, border->starty, border->startx);
    //显示边框
    box(local_win, 0, 0);
    wrefresh(local_win);

    return local_win;
}

//初始化窗口显示
void initWin(WINDOW *win, Block *block, Border border, int pile[][WIDTH]) {

    createBlock(win, block, border);

    //等待输入任意键开始游戏
    getch();

    //开始后用游戏操作提示覆盖游戏开始提示
    char *help = "^:spin  V:down  <:left  >:right  S:pause";
    mvprintw(LINES - 1, (COLS - strlen(help)) / 2, help);
    refresh();
}

//处理键盘控制
int dealControl(WINDOW *win, Block *block, int pile[][WIDTH], int *speed, Border border) {
    //获取键盘输入
    int ch = getch();

    if (ch != ERR) {   //有键盘输入

        //旋转，到底，左右移动，暂停
        switch (ch) {
            case 's':   //暂停
            case 'S':
                while (1) {
                    ch = getch();
                    if (ch == 's' || ch == 'S') {
                        break;
                    }
                }
                break;
            case KEY_LEFT:  //向左
                leftMove(win, block, pile, border);
                break;
            case KEY_RIGHT: //向右
                rightMove(win, block, pile, border);
                break;
            case KEY_UP:    //向上
                spin(win,block, pile, border);
                break;
            case KEY_DOWN:  //到底
                down(win, block, pile, border);
                break;
        }
        return 1;
    }

    return 0;
}

int checkLanding(WINDOW *win, Block *block, int pile[][WIDTH], Border border) {
    //碰到已落地方块或地面
    if (pile[block->b[0].y + 1][block->b[0].x] == 1 || pile[block->b[1].y + 1][block->b[1].x] == 1 ||
        pile[block->b[2].y + 1][block->b[2].x] == 1 || pile[block->b[3].y + 1][block->b[3].x] == 1 ||
        block->b[0].y + 1 == border.height - 1 || block->b[1].y + 1 == border.height - 1 ||
        block->b[2].y + 1 == border.height - 1 || block->b[3].y + 1 == border.height - 1) {
        //纳入已落定区域
        for (int i = 0; i < 4; ++i) {
            pile[block->b[i].y][block->b[i].x] = 1;
        }
        return 1;
    }

    return 0;
}

int checkCrash(int pile[][WIDTH], Border border) {
    for (int i = 1; i < border.width - 1; ++i) {
        if (pile[0][i] == 1) {
            return 1;
        }
    }
    return 0;
}

//TODO some problem when after clean first drop???
int checkEliminate(WINDOW *win, int pile[][WIDTH], Border border) {

    int lines = 0, count;

    for (int i = border.height - 2; i > 0; --i) {
        count = 0;
        for (int j = 1; j < border.width - 1; ++j) {
            if (pile[i][j] == 1) {
                count++;
            } else {
                break;
            }
        }
        if (count == border.width - 2) { //行满，其上依次下移覆盖
            for (int l = 1; l < border.width - 1; ++l) {
                pile[i][l]=0;
                mvwaddch(win, i, l, ' ');
                wrefresh(win);
                usleep(50000);
            }
            for (int k = i; k > 1; --k) {
                for (int l = 1; l < border.width - 1; ++l) {
                    pile[k][l] = pile[k - 1][l];
                    pile[k - 1][l]=0;
                    if (pile[k][l]) {
                        mvwaddch(win, k, l, '#');
                    }
                    mvwaddch(win, k - 1, l, ' ');
                }
            }
            wrefresh(win);
            //最上行置0
            //for (int l = 1; l < border.width - 1; ++l) {
            //    pile[1][l] = 0;
            //}
            lines++;
            i++;    //继续判断当前行
        }
    }

    return lines;
}

void createBlock(WINDOW *win, Block *block, Border border) {

    int offset = border.width / 2;
    int t = rand() % 20;
    switch (t) {
        case O:
            block->type = O;
            block->b[0].x = offset;
            block->b[0].y = 0;
            block->b[1].x = offset + 1;
            block->b[1].y = 0;
            block->b[2].x = offset;
            block->b[2].y = 1;
            block->b[3].x = offset + 1;
            block->b[3].y = 1;
            break;
        case S1:
            block->type = S1;
            block->b[0].x = offset + 1;
            block->b[0].y = 0;
            block->b[1].x = offset + 2;
            block->b[1].y = 0;
            block->b[2].x = offset;
            block->b[2].y = 1;
            block->b[3].x = offset + 1;
            block->b[3].y = 1;
            break;
        case S2:
            block->type = S2;
            block->b[0].x = offset;
            block->b[0].y = -1;
            block->b[1].x = offset;
            block->b[1].y = 0;
            block->b[2].x = offset + 1;
            block->b[2].y = 0;
            block->b[3].x = offset + 1;
            block->b[3].y = 1;
            break;
        case Z1:
            block->type = Z1;
            block->b[0].x = offset;
            block->b[0].y = 0;
            block->b[1].x = offset + 1;
            block->b[1].y = 0;
            block->b[2].x = offset + 1;
            block->b[2].y = 1;
            block->b[3].x = offset + 2;
            block->b[3].y = 1;
            break;
        case Z2:
            block->type = Z2;
            block->b[0].x = offset + 1;
            block->b[0].y = -1;
            block->b[1].x = offset;
            block->b[1].y = 0;
            block->b[2].x = offset + 1;
            block->b[2].y = 0;
            block->b[3].x = offset;
            block->b[3].y = 1;
            break;
        case L1:
            block->type = L1;
            block->b[0].x = offset;
            block->b[0].y = -1;
            block->b[1].x = offset;
            block->b[1].y = 0;
            block->b[2].x = offset;
            block->b[2].y = 1;
            block->b[3].x = offset + 1;
            block->b[3].y = 1;
            break;
        case L2:
            block->type = L2;
            block->b[0].x = offset;
            block->b[0].y = 0;
            block->b[1].x = offset + 1;
            block->b[1].y = 0;
            block->b[2].x = offset + 2;
            block->b[2].y = 0;
            block->b[3].x = offset;
            block->b[3].y = 1;
            break;
        case L3:
            block->type = L3;
            block->b[0].x = offset;
            block->b[0].y = -1;
            block->b[1].x = offset + 1;
            block->b[1].y = -1;
            block->b[2].x = offset + 1;
            block->b[2].y = 0;
            block->b[3].x = offset + 1;
            block->b[3].y = 1;
            break;
        case L4:
            block->type = L4;
            block->b[0].x = offset + 2;
            block->b[0].y = 0;
            block->b[1].x = offset;
            block->b[1].y = 1;
            block->b[2].x = offset + 1;
            block->b[2].y = 1;
            block->b[3].x = offset + 2;
            block->b[3].y = 1;
            break;
        case J1:
            block->type = J1;
            block->b[0].x = offset + 1;
            block->b[0].y = -1;
            block->b[1].x = offset + 1;
            block->b[1].y = 0;
            block->b[2].x = offset;
            block->b[2].y = 1;
            block->b[3].x = offset + 1;
            block->b[3].y = 1;
            break;
        case J2:
            block->type = J2;
            block->b[0].x = offset;
            block->b[0].y = 0;
            block->b[1].x = offset;
            block->b[1].y = 1;
            block->b[2].x = offset + 1;
            block->b[2].y = 1;
            block->b[3].x = offset + 2;
            block->b[3].y = 1;
            break;
        case J3:
            block->type = J3;
            block->b[0].x = offset;
            block->b[0].y = -1;
            block->b[1].x = offset + 1;
            block->b[1].y = -1;
            block->b[2].x = offset;
            block->b[2].y = 0;
            block->b[3].x = offset;
            block->b[3].y = 1;
            break;
        case J4:
            block->type = J4;
            block->b[0].x = offset;
            block->b[0].y = 0;
            block->b[1].x = offset + 1;
            block->b[1].y = 0;
            block->b[2].x = offset + 2;
            block->b[2].y = 0;
            block->b[3].x = offset + 2;
            block->b[3].y = 1;
            break;
        case I1:
            block->type = I1;
            block->b[0].x = offset;
            block->b[0].y = -2;
            block->b[1].x = offset;
            block->b[1].y = -1;
            block->b[2].x = offset;
            block->b[2].y = 0;
            block->b[3].x = offset;
            block->b[3].y = 1;
            break;
        case I2:
            block->type = I2;
            block->b[0].x = offset;
            block->b[0].y = 1;
            block->b[1].x = offset + 1;
            block->b[1].y = 1;
            block->b[2].x = offset + 2;
            block->b[2].y = 1;
            block->b[3].x = offset + 3;
            block->b[3].y = 1;
            break;
        case T1:
            block->type = T1;
            block->b[0].x = offset;
            block->b[0].y = 0;
            block->b[1].x = offset + 1;
            block->b[1].y = 0;
            block->b[2].x = offset + 2;
            block->b[2].y = 0;
            block->b[3].x = offset + 1;
            block->b[3].y = 1;
            break;
        case T2:
            block->type = T2;
            block->b[0].x = offset + 1;
            block->b[0].y = -1;
            block->b[1].x = offset;
            block->b[1].y = 0;
            block->b[2].x = offset + 1;
            block->b[2].y = 0;
            block->b[3].x = offset + 1;
            block->b[3].y = 1;
            break;
        case T3:
            block->type = T3;
            block->b[0].x = offset + 1;
            block->b[0].y = 0;
            block->b[1].x = offset;
            block->b[1].y = 1;
            block->b[2].x = offset + 1;
            block->b[2].y = 1;
            block->b[3].x = offset + 2;
            block->b[3].y = 1;
            break;
        case T4:
            block->type = T4;
            block->b[0].x = offset;
            block->b[0].y = -1;
            block->b[1].x = offset;
            block->b[1].y = 0;
            block->b[2].x = offset + 1;
            block->b[2].y = 0;
            block->b[3].x = offset;
            block->b[3].y = 1;
            break;
    }
}

int spin(WINDOW *win,Block *block, int pile[][WIDTH], Border border) {

    for (int i = 0; i < 4; ++i) {
        if (block->b[i].y>0){
            mvwaddch(win, block->b[i].y, block->b[i].x, ' ');
        }
    }
    wrefresh(win);

    switch (block->type) {
        case O:
            break;
        case S1:
            if (pile[block->b[0].y-1][block->b[0].x-1]==0&&pile[block->b[0].y][block->b[0].x-1]==0){
                block->type = S2;
                block->b[0].x -= 1;
                block->b[0].y -= 1;
                block->b[1].x -= 2;
                block->b[2].x += 1;
                block->b[2].y -= 1;
            }
            break;
        case S2:
            if(pile[block->b[1].y][block->b[0].x+2]==0&&pile[block->b[0].y+1][block->b[0].x]==0){
                block->type = S1;
                block->b[0].x += 1;
                block->b[0].y += 1;
                block->b[1].x += 2;
                block->b[2].x -= 1;
                block->b[2].y += 1;
            }

            break;
        case Z1:
            if (pile[block->b[1].y-1][block->b[0].x+1]==0&&pile[block->b[1].y][block->b[0].x+1]==0){
                block->type = Z2;
                block->b[0].x += 2;
                block->b[0].y -= 1;
                block->b[2].x += 1;
                block->b[2].y -= 1;
                block->b[3].x -= 1;
            }
            break;
        case Z2:
            if (pile[block->b[1].y-1][block->b[0].x+1]==0&&pile[block->b[1].y][block->b[0].x+1]==0){
                block->type = Z1;
                block->b[0].x -= 2;
                block->b[0].y += 1;
                block->b[2].x -= 1;
                block->b[2].y += 1;
                block->b[3].x += 1;
            }
            break;
        case L1:
            if (pile[block->b[0].y][block->b[0].x+1]==0&&pile[block->b[0].y][block->b[0].x+2]==0){
                block->type = L2;
                block->b[1].x += 1;
                block->b[1].y -= 1;
                block->b[2].x += 2;
                block->b[2].y -= 2;
                block->b[3].x -= 1;
                block->b[3].y -= 1;
            }
            break;
        case L2:
            if (pile[block->b[2].y+1][block->b[2].x]==0&&pile[block->b[2].y+2][block->b[2].x]==0){
                block->type = L3;
                block->b[0].x += 1;
                block->b[1].x += 1;
                block->b[2].y += 1;
                block->b[3].x += 2;
                block->b[3].y += 1;
            }
            break;
        case L3:
            if (pile[block->b[3].y][block->b[3].x-1]==0&&pile[block->b[3].y][block->b[3].x-2]==0){
                block->type = L4;
                block->b[0].x += 1;
                block->b[0].y += 1;
                block->b[1].x -= 2;
                block->b[1].y += 2;
                block->b[2].x -= 1;
                block->b[2].y += 1;
            }
            break;
        case L4:
            if (pile[block->b[1].y-1][block->b[1].x]==0&&pile[block->b[1].y-2][block->b[1].x]==0){
                block->type = L1;
                block->b[0].x -= 2;
                block->b[0].y -= 1;
                block->b[1].y -= 1;
                block->b[2].x -= 1;
                block->b[3].x -= 1;
            }
            break;
        case J1:
            if (pile[block->b[2].y][block->b[2].x-1]==0&&pile[block->b[2].y-1][block->b[2].x-1]==0){
                block->type = J2;
                block->b[0].x -= 2;
                block->b[0].y += 1;
                block->b[1].x -= 2;
                block->b[1].y += 1;
            }
            break;
        case J2:
            if (pile[block->b[0].y-1][block->b[0].x]==0&&pile[block->b[0].y-1][block->b[0].x+1]==0){
                block->type = J3;
                block->b[0].y -= 1;
                block->b[1].x += 1;
                block->b[1].y -= 2;
                block->b[2].x -= 1;
                block->b[2].y -= 1;
                block->b[3].x -= 2;
            }
            break;
        case J3:
            if (pile[block->b[1].y][block->b[1].x+1]==0&&pile[block->b[1].y+1][block->b[1].x+1]==0){
                block->type = J4;
                block->b[2].x += 2;
                block->b[2].y -= 1;
                block->b[3].x += 2;
                block->b[3].y -= 1;
            }
            break;
        case J4:
            if (pile[block->b[3].y-1][block->b[3].x]==0&&pile[block->b[3].y-1][block->b[3].x+1]==0){
                block->type = J1;
                block->b[0].x += 2;
                block->b[1].x += 1;
                block->b[1].y += 1;
                block->b[2].x -= 1;
                block->b[2].y += 2;
                block->b[3].y += 1;
            }
            break;
        case I1:
            if (pile[block->b[1].y][block->b[1].x-1]==0&&pile[block->b[1].y][block->b[1].x+1]==0&&pile[block->b[1].y][block->b[1].x+2]==0){
                block->type = I2;
                block->b[0].x -= 1;
                block->b[0].y += 1;
                block->b[2].x += 1;
                block->b[2].y -= 1;
                block->b[3].x += 2;
                block->b[3].y -= 2;
            }
            break;
        case I2:
            if (pile[block->b[1].y-1][block->b[1].x]==0&&pile[block->b[1].y+1][block->b[1].x]==0&&pile[block->b[1].y+2][block->b[1].x]==0){
                block->type = I1;
                block->b[0].x += 1;
                block->b[0].y -= 1;
                block->b[2].x -= 1;
                block->b[2].y += 1;
                block->b[3].x -= 2;
                block->b[3].y += 2;
            }
            break;
        case T1:
            if (pile[block->b[1].y-1][block->b[1].x]==0){
                block->type = T2;
                block->b[0].x += 1;
                block->b[0].y -= 1;
                block->b[1].x -= 1;
                block->b[2].x -= 1;
            }
            break;
        case T2:
            if (pile[block->b[2].y][block->b[2].x+1]==0){
                block->type = T3;
                block->b[3].x += 1;
                block->b[3].y -= 1;
            }
            break;
        case T3:
            if (pile[block->b[2].y+1][block->b[2].x]==0){
                block->type = T4;
                block->b[1].x += 1;
                block->b[2].x += 1;
                block->b[3].x -= 1;
                block->b[3].y += 1;
            }
            break;
        case T4:
            if (pile[block->b[1].y][block->b[1].x-1]==0){
                block->type = T1;
                block->b[0].x -= 1;
                block->b[0].y += 1;
            }
            break;
    }

    for (int i = 0; i < 4; ++i) {
        if (block->b[i].y>0){
            mvwaddch(win, block->b[i].y, block->b[i].x, '#');
        }
    }
    wrefresh(win);

    return 0;
}

//下落
void refreshBlock(WINDOW *win, Block *block) {
    for (int i = 0; i < 4; ++i) {
        if (block->b[i].y>0){
            mvwaddch(win, block->b[i].y, block->b[i].x, ' ');
        }
    }
    wrefresh(win);
    for (int i = 0; i < 4; ++i) {
        block->b[i].y++;
    }
    for (int i = 0; i < 4; ++i) {
        if (block->b[i].y>0){
            mvwaddch(win, block->b[i].y, block->b[i].x, '#');
        }
    }
    wrefresh(win);
}


void leftMove(WINDOW *win, Block *block, int pile[][WIDTH], Border border) {
    for (int i = 0; i < 4; ++i) {
        if (block->b[i].x - 1 < 1 || pile[block->b[i].y][block->b[i].x-1] == 1) {
            return;
        }
    }
    for (int i = 0; i < 4; ++i) {
        if (block->b[i].y>0){
            mvwaddch(win, block->b[i].y, block->b[i].x, ' ');
        }
    }
    wrefresh(win);
    for (int i = 0; i < 4; ++i) {
        block->b[i].x--;
    }
    for (int i = 0; i < 4; ++i) {
        if (block->b[i].y>0) {
            mvwaddch(win, block->b[i].y, block->b[i].x, '#');
        }
    }
    wrefresh(win);
}

void rightMove(WINDOW *win, Block *block, int pile[][WIDTH], Border border) {
    for (int i = 0; i < 4; ++i) {
        if (block->b[i].x + 1 > border.width - 2 || pile[block->b[i].y][block->b[i].x+1] == 1) {
            return;
        }
    }
    for (int i = 0; i < 4; ++i) {
        if (block->b[i].y>0){
            mvwaddch(win, block->b[i].y, block->b[i].x, ' ');
        }
    }
    wrefresh(win);
    for (int i = 0; i < 4; ++i) {
        block->b[i].x++;
    }
    for (int i = 0; i < 4; ++i) {
        if (block->b[i].y>0) {
            mvwaddch(win, block->b[i].y, block->b[i].x, '#');
        }
    }
    wrefresh(win);
}

void down(WINDOW *win, Block *block, int pile[][WIDTH], Border border) {

    while (!checkLanding(win, block, pile, border)) {
        refreshBlock(win, block);
    }

}

//销毁窗口
void destroyWin(WINDOW *win) {
    //显示退出游戏提示
    char *crash = "crash !";
    char *quit = "quiting...";

    attron(A_BOLD); //粗体文字
    mvprintw(LINES / 2, (COLS - strlen(crash)) / 2, crash);
    mvprintw(LINES / 2 + 1, (COLS - strlen(quit)) / 2, quit);
    attron(A_BOLD); //粗体文字
    refresh();

    //延时
    sleep(2);
    //销毁窗口
    delwin(win);
}