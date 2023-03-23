#include "PongGame.h"
#include "Keys.h"

static float DifficultyCurve(unsigned ballHits)
{
    return 1.0f + ballHits / 5.0f;
}

void PongGame::Update()
{
    Game::Update();

    ball->Speed = DifficultyCurve(ballHits);
    if (inputDevice->IsKeyDown(Keys::Escape))
        isExitRequested = true;
    float temp = ball->GetY() - racket1->GetY();
    if (temp > deltaTime * racket1->Speed)
        temp = deltaTime * racket1->Speed;
    if (temp < -deltaTime * racket1->Speed)
        temp = -deltaTime * racket1->Speed;
    
    racket1->SetY(racket1->GetY() + temp);
    racket2->SetY(-2.0 * mouseY / GetDisplay()->GetScreenHeight() + 1);

    WCHAR text[100];
    swprintf_s(text, TEXT("Left player score: %i ::: Right player score: %i"), s1, s2);
    SetWindowText(display->GetHWnd(), text);
 
}

PongGame::PongGame(LPCWSTR name, int screenWidth, int screenHeight) :
    Game(name, screenWidth, screenHeight)
{
    ballHits = 0;
}

void PongGame::ConfigureGameObjects() {
    racket1 = new RacketComponent();
    racket1->SetX(-0.9f);
    racket2 = new RacketComponent();
    racket2->SetX(0.9f);
    ball = new CircleComponent();
    components.push_back(racket1);
    components.push_back(racket2);
    components.push_back(ball);
    ContinueTime = std::chrono::steady_clock::now() + std::chrono::seconds(1);
}

void PongGame::CreateInstance(LPCWSTR name, int screenWidth, int screenHeight) {
    if (!instance)
        instance = new PongGame(name, screenWidth, screenHeight);
}

void PongGame::Draw()
{
    if (s1 > 5 || s2 > 5)
    {
        if (s1 > 3) {
            MessageBox(Game::instance->GetDisplay()->GetHWnd(), L"Left Player Won!", L"PongGame", MB_OK);
        } else {
            MessageBox(Game::instance->GetDisplay()->GetHWnd(), L"Right Player Won!", L"PongGame", MB_OK);
        }
        isExitRequested = true;
    }
    Game::Draw();
}

void PongGame::Initialize()
{
    ConfigureGameObjects();
    Game::Initialize();
}

void PongGame::Run() {
    Game::Run();
}

void PongGame::IncScore(bool p)
{
    if (p)
        s1 += 1;
    else
        s2 += 1;
}
