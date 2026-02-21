#include <windows.h>
#include <thread>
#include <atomic>
#include <memory>
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>

#include <boost/multiprecision/cpp_bin_float.hpp>

#ifdef USE_AVX
#include <immintrin.h>
#endif

using BigFloat = boost::multiprecision::number<
    boost::multiprecision::cpp_bin_float<16384>
>;

#define WM_UPDATE_VALUES (WM_APP + 1)

struct BaselState
{
    BigFloat sum = 0;
    BigFloat compensation = 0;
    uint64_t n = 1;
};

std::atomic<bool> running{ true };
std::atomic<std::shared_ptr<BaselState>> snapshotPtr;

HWND hTextN;
HWND hTextSum;
HWND hTextError;

BigFloat pi_squared_over_6()
{
    BigFloat pi = acos(BigFloat(-1));
    return (pi * pi) / 6;
}

void compute_block(BaselState& state, uint64_t iterations)
{
#ifdef USE_AVX

    for (uint64_t i = 0; i < iterations; i += 4)
    {
        __m256d n_vals = _mm256_set_pd(
            (double)(state.n + 3),
            (double)(state.n + 2),
            (double)(state.n + 1),
            (double)(state.n)
        );

        __m256d squared = _mm256_mul_pd(n_vals, n_vals);
        __m256d ones = _mm256_set1_pd(1.0);
        __m256d terms = _mm256_div_pd(ones, squared);

        double buffer[4];
        _mm256_storeu_pd(buffer, terms);

        for (int j = 0; j < 4; ++j)
        {
            BigFloat term = buffer[j];

            BigFloat y = term - state.compensation;
            BigFloat t = state.sum + y;
            state.compensation = (t - state.sum) - y;
            state.sum = t;

            state.n++;
        }
    }

#else

    for (uint64_t i = 0; i < iterations; ++i)
    {
        BigFloat term = BigFloat(1) / (BigFloat(state.n) * state.n);

        BigFloat y = term - state.compensation;
        BigFloat t = state.sum + y;
        state.compensation = (t - state.sum) - y;
        state.sum = t;

        state.n++;
    }

#endif
}

void WorkerThread(HWND hwnd)
{
    BaselState state;
    BigFloat target = pi_squared_over_6();

    auto nextUI = std::chrono::steady_clock::now();
    auto nextError = std::chrono::steady_clock::now();

    while (running)
    {
        compute_block(state, 5000);

        auto now = std::chrono::steady_clock::now();

        if (now >= nextUI)
        {
            snapshotPtr.store(std::make_shared<BaselState>(state));
            PostMessage(hwnd, WM_UPDATE_VALUES, 0, 0);
            nextUI += std::chrono::milliseconds(33);
        }

        if (now >= nextError)
        {
            nextError += std::chrono::seconds(1);
        }
    }
}

std::wstring scientific_notation(const BigFloat& x)
{
    std::ostringstream oss;
    oss << std::scientific << std::setprecision(20) << x;
    std::string s = oss.str();
    return std::wstring(s.begin(), s.end());
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        hTextN = CreateWindow(L"STATIC", L"", WS_CHILD | WS_VISIBLE,
            20, 20, 900, 25, hwnd, NULL, NULL, NULL);

        hTextSum = CreateWindow(L"STATIC", L"", WS_CHILD | WS_VISIBLE,
            20, 60, 900, 25, hwnd, NULL, NULL, NULL);

        hTextError = CreateWindow(L"STATIC", L"", WS_CHILD | WS_VISIBLE,
            20, 100, 900, 25, hwnd, NULL, NULL, NULL);
        break;

    case WM_UPDATE_VALUES:
    {
        auto snap = snapshotPtr.load();
        if (!snap) break;

        std::wstringstream ws;

        ws << L"n = " << snap->n;
        SetWindowText(hTextN, ws.str().c_str());

        std::string sumStr = snap->sum.str(50);
        std::wstring sumW(sumStr.begin(), sumStr.end());
        SetWindowText(hTextSum, sumW.c_str());

        static BigFloat target = pi_squared_over_6();
        BigFloat error = target - snap->sum;

        std::wstring errW = L"Error: " + scientific_notation(error);
        SetWindowText(hTextError, errW.c_str());
    }
    break;

    case WM_DESTROY:
        running = false;
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"BaselWin32";

    RegisterClass(&wc);

    HWND hwnd = CreateWindow(
        L"BaselWin32",
        L"16384-bit Infinite Basel Calculator",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 1000, 200,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);

    snapshotPtr.store(std::make_shared<BaselState>());

    std::thread worker(WorkerThread, hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    worker.join();
    return 0;
}
