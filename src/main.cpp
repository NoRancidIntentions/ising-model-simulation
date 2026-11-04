//The main code, with visualisation. see varyingTemp.cpp to see variance of M vs T

#include <SFML/Graphics.hpp>
#include <Random123/philox.h>
#include <Random123/u01fixedpt.h>

#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <thread>

using namespace r123;
using namespace std;

// L is the base-2 exponent of the lattice side length: side = 2^L.
const int L = 8;
const int grid_length = 1 << L;
const int N = grid_length * grid_length;

const double kb = 1.0;
double T = 0.5;
double Beta = 1.0 / (kb * T);

float alignCount = 0;
int totCalls = 0;
int totFlips = 0;
long long M = 0;
long long M2 = 0;

Philox4x32 rng;
array<int8_t, N> grid;

// Return four reproducible random doubles in the open interval (0, 1).
// Philox is counter-based: the counter tuple identifies each draw.
void getRandArray(double *r, uint32_t arg1, uint32_t arg2,
                  uint32_t arg3, uint32_t arg4 = 0)
{
    Philox4x32::ctr_type counter = {{arg1, arg2, arg3, arg4}};
    Philox4x32::key_type seed = {{166463}};

    auto randArray = rng(counter, seed);

    for (uint8_t i = 0; i < 4; i++)
    {
        r[i] = u01fixedpt_open_open_32_double(randArray[i]);
    }
}

void printGrid()
{
    for (int i = 0; i < N; i++)
    {
        cout << static_cast<int>(grid[i]) << " ";
        if (((i + 1) % grid_length) == 0)
        {
            cout << '\n';
        }
    }
}

inline int8_t getGridElement(int x, int y)
{
    return grid[x + y * grid_length];
}

void updatePixel(int x, int y, uint32_t index)
{
    Beta = 1.0 / (kb * T);
    totCalls++;

    // Periodic (toroidal) boundary conditions. The bit mask is valid because
    // grid_length is a power of two.
    int xm = (x - 1 + grid_length) & (grid_length - 1);
    int xp = (x + 1) & (grid_length - 1);
    int ym = (y - 1 + grid_length) & (grid_length - 1);
    int yp = (y + 1) & (grid_length - 1);

    int neighSpin = static_cast<int>(getGridElement(xm, y) +
                                     getGridElement(xp, y) +
                                     getGridElement(x, ym) +
                                     getGridElement(x, yp));
    int dE = 2 * getGridElement(x, y) * neighSpin;

    if (dE < 0)
    {
        grid[x + y * grid_length] = -grid[x + y * grid_length];
        alignCount++;
        totFlips++;
    }
    else if (dE == 0)
    {
        double rand[4];
        getRandArray(rand, x, y, index, 2);

        if (rand[0] < 0.5)
        {
            alignCount += 0.5;
            grid[x + y * grid_length] = -grid[x + y * grid_length];
            totFlips++;
        }
    }
    else
    {
        // Energetically unfavourable flips are accepted with exp(-Beta * dE).
        double r[4];
        getRandArray(r, x, y, index, 0);

        if (r[0] < exp(-Beta * dE))
        {
            grid[x + y * grid_length] = -grid[x + y * grid_length];
            totFlips++;
        }
    }
}

int main()
{
    ofstream file("Ising.csv");
    if (!file)
    {
        cerr << "Could not open Ising.csv for writing.\n";
        return 1;
    }

    file << "T, aligCnt/totCalls, aligCnt/totFlips, varM\n";

    // Each iteration writes four distinct lattice entries. The RNG is
    // counter-based, so no shared mutable RNG state is used in this loop.
#pragma omp parallel for
    for (int i = 0; i < N; i += 4)
    {
        double rand[4];
        getRandArray(rand, static_cast<uint32_t>(i), 0, 0, 0);

        for (uint8_t j = 0; j < 4; j++)
        {
            grid[i + j] = (rand[j] < 0.5) ? static_cast<int8_t>(-1)
                                          : static_cast<int8_t>(1);
        }
    }

    constexpr float scale = 2.0f;
    sf::RenderWindow window(
        sf::VideoMode({static_cast<unsigned int>(grid_length * scale),
                       static_cast<unsigned int>(grid_length * scale)}),
        "Ising");

    static uint32_t global_time = 0;
    static bool j = false;
    int cycleCount = 0;

    while (window.isOpen())
    {
        window.setFramerateLimit(60);
        cycleCount++;

        while (const optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
        }

        window.clear();
        sf::VertexArray points(sf::PrimitiveType::Points, N);
        long long thisM = 0;

        for (int i = 0; i < N; i++)
        {
            int x = i & (grid_length - 1);
            int y = i >> L;
            points[i].position = sf::Vector2f({x * scale, y * scale});

            const sf::Color spinColor = (getGridElement(x, y) == 1)
                                            ? sf::Color::White
                                            : sf::Color::Black;
            if (getGridElement(x, y) == 1)
            {
                thisM++;
            }
            else
            {
                thisM--;
            }

            sf::Vertex vertices[]{
                {{x * scale, y * scale}, spinColor},
                {{(x + 1) * scale, y * scale}, spinColor},
                {{(x + 1) * scale, (y + 1) * scale}, spinColor},
                {{x * scale, y * scale}, spinColor},
                {{x * scale, (y + 1) * scale}, spinColor},
                {{(x + 1) * scale, (y + 1) * scale}, spinColor},
            };
            window.draw(vertices, 6, sf::PrimitiveType::Triangles);
            points[i].color = spinColor;
        }

        // The code records the absolute magnetization |M|, not signed M.
        thisM = std::abs(thisM);
        M2 += thisM * thisM;
        M += thisM;

        window.draw(points);
        window.display();

        const float favourablePerCall = static_cast<float>(alignCount) / totCalls;
        const float favourablePerFlip = (totFlips > 0)
                                            ? static_cast<float>(alignCount) / totFlips
                                            : 0.0f;
        cout << "\nfavAliCnt/totUpds= " << favourablePerCall;
        cout << "    nfavAliCnt/totFlips = " << favourablePerFlip;

        long double avgM = static_cast<long double>(M) / cycleCount;
        long double avg_M2 = static_cast<long double>(M2) / cycleCount;
        long double varM = avg_M2 - avgM * avgM;
        cout << "   varM= " << varM;

        file << '\n' << T << ", " << favourablePerCall << ", "
             << favourablePerFlip << ", " << varM;
        file.flush();

        if (cycleCount == 1)
        {
            this_thread::sleep_for(chrono::seconds(7));
        }

        // Checkerboard Metropolis sweep: update one parity class, then the
        // opposite class. Neighbour spins are fixed during each half-sweep.
        for (int i = 0; i < 1; i++)
        {
            for (int k = 0; k < N; k++)
            {
                int x = k & (grid_length - 1);
                int y = k >> L;

                if (((x ^ y) & 1) == j)
                {
                    updatePixel(x, y, global_time * N + k);
                }
            }
            j = !j;
            global_time++;
        }
    }

    return 0;
}
