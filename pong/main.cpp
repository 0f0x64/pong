//linker::system::subsystem  - Windows(/ SUBSYSTEM:WINDOWS)
//configuration::advanced::character set - not set
//linker::input::additional dependensies Msimg32.lib; Winmm.lib

#include "windows.h"
#include "math.h"
#include "stdio.h"


// секция данных игры  
typedef struct {
    float x, y, width, height, rad, dx, dy, speed;
    HBITMAP hBitmap;//хэндл к спрайту шарика 
    bool active;
} sprite;

sprite racket;//ракетка игрока
sprite ball;//шарик
const int brickCountX = 20;
const int brickCountY = 5;
sprite brick[brickCountX][brickCountY];//кирпичи


struct {
    int score, balls;//количество набранных очков и оставшихся "жизней"
    bool action = false;//состояние - ожидание (игрок должен нажать пробел) или игра
} game;

struct {
    HWND hWnd;//хэндл окна
    HDC device_context, context;// два контекста устройства (для буферизации)
    int width, height;//сюда сохраним размеры окна которое создаст программа
} window;

HBITMAP hBack;// хэндл для фонового изображения

//cекция кода

void InitGame()
{
    //в этой секции загружаем спрайты с помощью функций gdi
    //пути относительные - файлы должны лежать рядом с .exe 
    //результат работы LoadImageA сохраняет в хэндлах битмапов, рисование спрайтов будет произовдиться с помощью этих хэндлов
    ball.hBitmap = (HBITMAP)LoadImageA(NULL, "ball.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    racket.hBitmap = (HBITMAP)LoadImageA(NULL, "racket.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    hBack = (HBITMAP)LoadImageA(NULL, "back.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    //------------------------------------------------------

    racket.width = 300;
    racket.height = 50;
    racket.speed = 50;//скорость перемещения ракетки
    racket.x = window.width / 2.;//ракетка посередине окна
    racket.y = window.height - racket.height;//чуть выше низа экрана - на высоту ракеткиf

    ball.dy = (rand() % 65 + 35) / 100.;//формируем вектор полета шарика
    ball.dx = (1 - ball.dy);//формируем вектор полета шарика
    ball.speed = 100;
    ball.rad = 20;
    ball.x = racket.x;//x координата шарика - на середие ракетки
    ball.y = racket.y - ball.rad;//шарик лежит сверху ракетки

    game.score = 0;
    game.balls = 9;

    for (int y = 0; y < brickCountY; y++)
    {
        for (int x = 0; x < brickCountX; x++)
        {
            brick[x][y].hBitmap = racket.hBitmap;
            brick[x][y].width = 120;
            brick[x][y].x = x * brick[x][y].width;
            brick[x][y].height = 80;
            brick[x][y].y = y * brick[x][y].height;
            brick[x][y].active = true;
            if (y < 3)
                brick[x][y].active = false;
        }
    }

}

void ProcessSound(const char* name)//проигрывание аудиофайла в формате .wav, файл должен лежать в той же папке где и программа
{
    PlaySound(TEXT(name), NULL, SND_FILENAME | SND_ASYNC);//переменная name содежрит имя файла. флаг ASYNC позволяет проигрывать звук паралельно с исполнением программы
}

void ShowScore() //ShowScore - метод.
{
    //поиграем шрифтами и цветами
    SetTextColor(window.context, RGB(160, 160, 160));
    SetBkColor(window.context, RGB(0, 0, 0));
    SetBkMode(window.context, TRANSPARENT);
    auto hFont = CreateFont(70, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, "CALIBRI");
    auto hTmp = (HFONT)SelectObject(window.context, hFont);

    char txt[32];//буфер для текста
    _itoa_s(game.score, txt, 10);//преобразование числовой переменной в текст. текст окажется в переменной txt
    TextOutA(window.context, 10, 10, "Score", 5);
    TextOutA(window.context, 200, 10, (LPCSTR)txt, strlen(txt));

    _itoa_s(game.balls, txt, 10);
    TextOutA(window.context, 10, 100, "Balls", 5);
    TextOutA(window.context, 200, 100, (LPCSTR)txt, strlen(txt));
}

void ProcessInput()
{
    if (GetAsyncKeyState(VK_LEFT)) racket.x -= racket.speed;
    if (GetAsyncKeyState(VK_RIGHT)) racket.x += racket.speed;

    if (!game.action && GetAsyncKeyState(VK_SPACE))
    {
        game.action = true;
        ProcessSound("bounce.wav");
    }
}

void ShowBitmap(HDC hDC, int x, int y, int x1, int y1, HBITMAP hBitmapBall, bool alpha = false)
{
    HBITMAP hbm, hOldbm;
    HDC hMemDC;
    BITMAP bm;

    hMemDC = CreateCompatibleDC(hDC); // Создаем контекст памяти, совместимый с контекстом отображения
    hOldbm = (HBITMAP)SelectObject(hMemDC, hBitmapBall);// Выбираем изображение bitmap в контекст памяти

    if (hOldbm) // Если не было ошибок, продолжаем работу
    {
        GetObject(hBitmapBall, sizeof(BITMAP), (LPSTR)&bm); // Определяем размеры изображения

        if (alpha)
        {
            TransparentBlt(window.context, x, y, x1, y1, hMemDC, 0, 0, x1, y1, RGB(0, 0, 0));//все пиксели черного цвета будут интепретированы как прозрачные
        }
        else
        {
            StretchBlt(hDC, x, y, x1, y1, hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY); // Рисуем изображение bitmap
        }

        SelectObject(hMemDC, hOldbm);// Восстанавливаем контекст памяти
    }

    DeleteDC(hMemDC); // Удаляем контекст памяти
}

void ShowRacketAndBall()
{
    ShowBitmap(window.context, 0, 0, window.width, window.height, hBack);//задний фон
    ShowBitmap(window.context, racket.x - racket.width / 2., racket.y, racket.width, racket.height, racket.hBitmap);// ракетка игрока
    ShowBitmap(window.context, ball.x - ball.rad, ball.y - ball.rad, 2 * ball.rad, 2 * ball.rad, ball.hBitmap, true);// шарик

    //ShowBitmap(window.context, ball.x - ball.rad+ball.dx*ball.speed, ball.y - ball.rad+ball.dy*ball.speed, 2 * ball.rad, 2 * ball.rad, ball.hBitmap, true);// шарик

    for (int y = 0; y < brickCountY; y++)
    {
        for (int x = 0; x < brickCountX; x++)
        {
            if (brick[x][y].active) //Отрисовка кирпичей?
            {
                ShowBitmap(window.context, brick[x][y].x, brick[x][y].y, brick[x][y].width, brick[x][y].height, brick[x][y].hBitmap);
            }
        }
    }
}

void LimitRacket()
{
    racket.x = max(racket.x, racket.width / 2.);//если коодината левого угла ракетки меньше нуля, присвоим ей ноль
    racket.x = min(racket.x, window.width - racket.width / 2.);//аналогично для правого угла
}

void CheckWalls()
{
    if (ball.x < ball.rad || ball.x > window.width - ball.rad)
    {
        ball.dx *= -1;
        ProcessSound("bounce.wav");
    }
}

void CheckRoof()
{
    if (ball.y < ball.rad)
    {
        ball.dy *= -1;
        ProcessSound("bounce.wav");
    }
}

bool tail = false;

void CheckFloor()
{
    if (ball.y > window.height - ball.rad - racket.height)//шарик пересек линию отскока - горизонталь ракетки
    {
        if (!tail && ball.x >= racket.x - racket.width / 2. - ball.rad && ball.x <= racket.x + racket.width / 2. + ball.rad)//шарик отбит, и мы не в режиме обработки хвоста
        {
            game.score++;//за каждое отбитие даем одно очко
            ball.speed += 5. / game.score;//но увеличиваем сложность - прибавляем скорости шарику
            ball.dy *= -1;//отскок
            racket.width -= 10. / game.score;//дополнительно уменьшаем ширину ракетки - для сложности
            ProcessSound("bounce.wav");//играем звук отскока
        }
        else
        {//шарик не отбит

            tail = true;//дадим шарику упасть ниже ракетки

            if (ball.y - ball.rad > window.height)//если шарик ушел за пределы окна
            {
                game.balls--;//уменьшаем количество "жизней"

                ProcessSound("fail.wav");//играем звук

                if (game.balls < 0) { //проверка условия окончания "жизней"

                    MessageBoxA(window.hWnd, "game over", "", MB_OK);//выводим сообщение о проигрыше
                    InitGame();//переинициализируем игру
                }

                ball.dy = -(rand() % 65 + 35) / 100.;//задаем новый случайный вектор для шарика
                ball.dx = (1 - ball.dy);
                ball.x = racket.x;//инициализируем координаты шарика - ставим его на ракетку
                ball.y = racket.y - ball.rad;
                game.action = false;//приостанавливаем игру, пока игрок не нажмет пробел
                tail = false;
            }
        }
    }
}

void clearScreen()
{
    RECT rect;
    GetClientRect(window.hWnd, &rect);
    auto blackBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(window.context, &rect, blackBrush);
    DeleteObject(blackBrush);
}
struct point2d
{
    float x;
    float y;
    float z;
};

point2d Line[] = {-1,1,1,1,1,1,1,-1,1,-1,-1,1,-1,1,-1,1,1,-1,1,-1,1,-1,-1,-1};

struct index
{
    int i[2];
};

index List[] = { 0,1,1,2,2,3,3,0,0,4,4,5,5,8,8,7,7,3,3,2,2,8,8,5,5,1,1,0,0,4,4,7 };

void DrawLine(point2d p1, point2d p2)
{
    MoveToEx(window.context, p1.x*50 + window.width / 2, p1.y*50 + window.height / 2 , NULL);
    LineTo(window.context, p2.x*50 + window.width / 2 , p2.y*50 + window.height / 2 );
}

void turnZ(point2d &p, float alpha)
{
    float x = p.x * cos(alpha) - p.y * sin(alpha);
    float y = p.x * sin(alpha) + p.y * cos(alpha);

    p.x = x;
    p.y = y;
}

void turnY(point2d& p, float alpha)
{
    float x = p.x * cos(alpha) - p.z * sin(alpha);
    float z = p.x * sin(alpha) + p.z * cos(alpha);

    p.x = x;
    p.z = z;
}

void turnX(point2d& p, float alpha)
{
    float y = p.y * cos(alpha) - p.z * sin(alpha);
    float z = p.y * sin(alpha) + p.z * cos(alpha);

    p.y = y;
    p.z = z;
}
void Marker()
{
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));

    SelectObject(window.context, pen);
    clearScreen();
    
    float alpha = timeGetTime() * .001;

    for (int i = 0;i < 16; i++)
    {
        int ind1 = List[i].i[0];
        int ind2 = List[i].i[1];

        point2d p1 = Line[ind1];
        point2d p2 = Line[ind2];
        turnX(p1, alpha);
        turnX(p2, alpha);

        DrawLine(p1, p2);
    }
    DeleteObject(pen);
}


bool collision;

void ProcessRoom()
{
    collision = false;

    //обрабатываем стены, потолок и пол. принцип - угол падения равен углу отражения, а значит, для отскока мы можем просто инвертировать часть вектора движения шарика
    CheckWalls();
    CheckRoof();
    CheckFloor();
    // Проверяем столкновения с кирпичами

    float curX = ball.x;
    float curY = ball.y;
    float curDX = ball.dx;
    float curDY = ball.dy;
    float tempY;
    float tempX;
    float br = ball.rad;
    float bdx = curDX * ball.speed;
    float bdy = curDY * ball.speed;
    int lenght = sqrt(bdx * bdx + bdy * bdy) / 2;
    bool n = false;
    for (int i = 0; i < lenght; i++)
    {


        float s = (float)i / (float)lenght;
        int n = 0;

        float strt = atan2(curDY, curDX) * (180. / 3.141519) - 90;
        //float angle = atan2(curDY, curDX) * (180. / 3.141519);
        //float strt = angle;
        for (float angle = strt;angle <= strt + 180;angle += 10)
        {

            for (int y = 0; y < brickCountY; y++)
            {
                for (int x = 0; x < brickCountX; x++)
                {

                    if (brick[x][y].active) // Если кирпич активен
                    {

                        tempX = curX + curDX * ball.speed * s;
                        tempY = curY + curDY * ball.speed * s;

                        float bx = tempX + br * cos(angle * 3.141519 / 180.);
                        float by = tempY + br * sin(angle * 3.141519 / 180.);
                        SetPixel(window.context, bx, by, RGB(255, 255, 255));

                        // Проверяем столкновение с кирпичом
                        if (bx >= brick[x][y].x && bx < brick[x][y].x + brick[x][y].width &&
                            by >= brick[x][y].y && by < brick[x][y].y + brick[x][y].height)
                        {

                            collision = true;
                            // Определяем, с какой гранью кирпича произошло столкновение
                            float overlapLeft = tempX - brick[x][y].x; // расстояние до левой грани кирпича
                            float overlapRight = brick[x][y].x + brick[x][y].width - tempX; // расстояние до правой грани кирпича
                            float overlapTop = tempY - brick[x][y].y; // расстояние до верхней грани кирпича
                            float overlapBottom = brick[x][y].y + brick[x][y].height - tempY; // расстояние до нижней грани кирпича
                            float edge;
                            float minV = min(overlapTop, overlapBottom);
                            float minH = min(overlapLeft, overlapRight);

                            if (minH < minV)
                            {
                                edge = -1.f;
                                curX += 2 * (tempX - curX);
                            }
                            if (minH > minV)
                            {
                                edge = 1.f;
                                curY += 2 * (tempY - curY);
                            }


                            curDX *= edge;
                            curDY *= -edge;

                            angle = strt + 180;
                            strt = atan2(curDY, curDX) * (180. / 3.141519) - 90;
                            brick[x][y].active = false; // Деактивируем кирпич
                            game.score++; // Увеличиваем счет
                            ProcessSound("bounce.wav"); // Проигрываем звук
                            n = false;

                        }
                    }
                }
            }
        }
    }


    if (collision)
    {

        ball.x = tempX;
        ball.y = tempY;
        ball.dx = curDX;
        ball.dy = curDY;
    }
}



void ProcessBall()
{
    if (game.action)
    {
        if (collision) return;
        //если игра в активном режиме - перемещаем шарик
        ball.x += ball.dx * ball.speed;
        ball.y += ball.dy * ball.speed;
    }
    else
    {
        //иначе - шарик "приклеен" к ракетке
        ball.x = racket.x;
    }
}

void InitWindow()
{
    SetProcessDPIAware();
    window.hWnd = CreateWindow("edit", 0, WS_POPUP | WS_VISIBLE | WS_MAXIMIZE, 0, 0, 0, 0, 0, 0, 0, 0);

    RECT r;
    GetClientRect(window.hWnd, &r);
    window.device_context = GetDC(window.hWnd);//из хэндла окна достаем хэндл контекста устройства для рисования
    window.width = r.right - r.left;//определяем размеры и сохраняем
    window.height = r.bottom - r.top;
    window.context = CreateCompatibleDC(window.device_context);//второй буфер
    SelectObject(window.context, CreateCompatibleBitmap(window.device_context, window.width, window.height));//привязываем окно к контексту
    GetClientRect(window.hWnd, &r);

}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{

    InitWindow();//здесь инициализируем все что нужно для рисования в окне
    InitGame();//здесь инициализируем переменные игры

    // mciSendString(TEXT("play ..\\Debug\\music.mp3 repeat"), NULL, 0, NULL);
    ShowCursor(NULL);

    while (!GetAsyncKeyState(VK_ESCAPE))
    {
        POINT p;
        GetCursorPos(&p);
        ScreenToClient(window.hWnd, &p);
        //ball.x = p.x;
        //ball.y = p.y;
        //ShowRacketAndBall();//рисуем фон, ракетку и шарик
        //ShowScore();//рисуем очик и жизни
        //ProcessRoom();//обрабатываем отскоки от стен и каретки, попадание шарика в картетку
        //ProcessBall();//перемещаем шарик
        Marker();

        BitBlt(window.device_context, 0, 0, window.width, window.height, window.context, 0, 0, SRCCOPY);//копируем буфер в окно
        Sleep(16);//ждем 16 милисекунд (1/количество кадров в секунду)

//        ProcessInput();//опрос клавиатуры
  //      LimitRacket();//проверяем, чтобы ракетка не убежала за экран

    }

}