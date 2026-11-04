# 2D Ising Model — Monte Carlo Simulation

A C++ visualization of the ferromagnetic two-dimensional square-lattice Ising model. The simulation uses single-spin Metropolis updates, a checkerboard update scheme, periodic boundary conditions, Random123 Philox random-number generation, and SFML 3 for real-time lattice visualization.

**Project status:** Learning / exploratory computational-physics project. This is a small simulation intended for studying Monte Carlo methods and phase transitions, not a production-grade Monte Carlo package.

## Project Timeline

- **Started:** June 2026

## 1. The Ising Model

### It is suggested to go through the Ising Model.pptx file for better explanation of the theory.



Each lattice site carries a spin variable

$$
s_i \in \{-1,+1\}.
$$

For a nearest-neighbour ferromagnetic interaction with zero external magnetic field, the Hamiltonian is

$$
\mathcal{H}=-J\sum_{\langle i,j\rangle}s_i s_j,
$$

where each nearest-neighbour bond is counted exactly once and $J>0$ is the coupling constant.

The energy change associated with flipping a spin at site $i$ is

$$
\Delta E_i=2Js_i\sum_{j\in\mathrm{nn}(i)}s_j.
$$

The Metropolis algorithm accepts a proposed spin flip with probability

$$
P_{\mathrm{acc}}=\min\left(1,e^{-\beta\Delta E_i}\right),
\qquad
\beta=\frac{1}{k_B T}.
$$

The implementation handles the three possible cases explicitly:

- **Favourable flip:** $\Delta E<0$. Always accepted.
- **Zero-energy flip:** $\Delta E=0$. Accepted with probability $1/2$.
- **Unfavourable flip:** $\Delta E>0$. Accepted with probability $e^{-\beta\Delta E}$.

The current implementation uses natural simulation units,

$$
J=k_B=1,
$$

with an initial temperature of `T = 0.5`.

## 2. Lattice and Update Scheme

### Lattice Configuration

- The system is a two-dimensional square lattice with periodic (toroidal) boundary conditions. Opposite edges are connected, so there are no physical boundaries.
- The lattice size is specified through an exponent:

  $$
  L=8 \quad\Longrightarrow\quad N_{\mathrm{side}}=2^8=256.
  $$

  Therefore, the total number of spins is

  $$
  N=256^2=65,536.
  $$

### Checkerboard Updates

The lattice is divided into two parity classes according to

$$
c(x,y)=(x+y)\bmod 2.
$$

The implementation updates one parity class while keeping the other fixed, then exchanges the two classes.

For an even-sized square lattice with nearest-neighbour interactions, sites belonging to the same parity class are not direct neighbours. Consequently, spins within one class can be updated independently, making the checkerboard scheme suitable for parallel execution.

### Random-Number Generation

The simulation uses the counter-based **Philox4x32** random-number generator from Random123.

- **Key (seed):** `166463`
- Random-number counters are constructed using the lattice-site coordinates and update index.
- The counter-based approach allows reproducible random-number generation without maintaining a shared mutable RNG state.

## 3. Connection to the High-Temperature Expansion

The Ising model also admits a graphical representation through its high-temperature expansion, which connects the spin formulation to closed-loop configurations.

Defining

$$
K=\beta J,
\qquad
u=\tanh K,
$$

the interaction term can be expanded as

$$
e^{Ks_i s_j} = \cosh K\left(1+u s_i s_j\right).
$$

Substituting this into the partition function,

$$
\mathcal{Z} =
\sum_{\{s_i\}}
\prod_{\langle i,j\rangle}e^{Ks_i s_j},
$$

gives

$$
\mathcal{Z}
= (\cosh K)^{N_b}
\sum_{\{s_i\}}
\prod_{\langle i,j\rangle}
\left(1+u s_i s_j\right),
$$

where $N_b$ is the total number of nearest-neighbour bonds.

Expanding the product corresponds to selecting subsets of bonds. When the sum over spin configurations is performed, every term containing an odd power of any spin vanishes:

$$
\sum_{s_i=\pm1}s_i^{n}
= \begin{cases}
2, & n\text{ even},\\
0, & n\text{ odd}.
\end{cases}
$$

Thus, only bond configurations with even degree at every vertex survive. These configurations form closed loops or unions of closed loops.

The partition function can therefore be written as

$$
\boxed{
\mathcal{Z}
= 2^N(\cosh K)^{N_b}
\sum_{G:\,\partial G=0}u^{|G|}
}
$$

where:

- $N$ is the number of lattice sites.
- $N_b$ is the number of bonds.
- $G$ represents a selected bond subgraph.
- $|G|$ is the number of selected bonds.
- $\partial G=0$ imposes the even-degree condition at every vertex.

On a periodic lattice (torus), the allowed configurations also include loops that wind around the periodic directions.

This graphical representation is the basis of the high-temperature expansion discussed in the accompanying Ising model presentation.

### Critical Temperature

For the infinite, zero-field, square-lattice Ising model, the exact critical temperature obtained by Onsager is

$$
\frac{k_B T_c}{J}
= \frac{2}{\ln(1+\sqrt{2})}
\approx 2.269185.
$$

The presentation's numerical experiments show a fluctuation peak around

$$
T\approx 2.26.
$$

This is qualitatively consistent with the exact critical temperature. However, a fluctuation peak from a finite simulation is not, by itself, a precision estimate of $T_c$.

## 4. Observables and Diagnostics

At each rendered frame, the program calculates the signed magnetization

$$
M=\sum_i s_i
$$

and records its absolute value,

$$
|M|.
$$

The running quantity printed as `varM` is

$$
\operatorname{var}(|M|) = \left\langle |M|^2\right\rangle_{\mathrm{run}}
- \left\langle |M|\right\rangle_{\mathrm{run}}^2.
$$

The averages are accumulated over the frames observed since the program started, at a fixed temperature.

**Important:** This is the time-series variance of the absolute magnetization. It is not the magnetic susceptibility, specific heat, or a temperature-scan result.

The `alignCount` diagnostics track accepted energy-lowering flips and half-weighted accepted zero-energy flips. These are implementation-specific diagnostics rather than standard thermodynamic observables.

## 5. Build and Run (Ubuntu)

### Dependencies

- C++20-compatible compiler
- CMake
- SFML 3
- OpenMP
- Random123 headers

Install the dependencies:

```bash
sudo apt update
sudo apt install build-essential cmake libsfml-dev librandom123-dev
```

**Note:** The project requires SFML 3. If your distribution provides SFML 2, install or build SFML 3 separately before configuring the project.

### Compilation

From the project root directory:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### Execution

```bash
./build/ising_model
```

The program generates an `Ising.csv` file in its working directory, containing:

- Temperature
- Update diagnostics
- Running magnetization variance (`varM`)

The SFML window provides a real-time visualization of the lattice:

- **White squares:** $s_i=+1$
- **Black squares:** $s_i=-1$

## 6. Current Limitations and Future Improvements

1. **Fixed temperature:** The temperature is currently hard-coded (`T = 0.5`). Exploring temperature dependence requires changing the value and restarting the simulation.

2. **Thermalisation:** The current running statistics include the initial transient. A more rigorous study should introduce an explicit thermalisation or burn-in period before collecting measurements.

3. **Statistical sampling:** Measurements should be collected at suitably separated intervals to reduce the effects of temporal autocorrelation. Independent runs with different random seeds would also help quantify statistical uncertainty.

4. **Thermodynamic observables:** A more complete implementation should accumulate moments of the signed magnetization and energy, including $M$, $M^2$, $M^4$, $E$, and $E^2$. These can be used to calculate standard observables such as magnetic susceptibility, specific heat, and Binder cumulants.

5. **Finite-size scaling:** Simulations at multiple lattice sizes would allow a more systematic investigation of critical behaviour and finite-size effects.

6. **Update-scheme validation:** A controlled comparison between standard random-site Metropolis updates and checkerboard updates would help verify that both implementations produce consistent equilibrium observables.

## 7. References

1. Ferrenberg, A. M., Landau, D. P., & Wong, Y. J. (1992). *Monte Carlo simulations: Hidden errors from "good" random number generators.* Physical Review Letters, **69**, 3382.  
   https://doi.org/10.1103/PhysRevLett.69.3382

2. **Random123** — Counter-based random-number generators.  
   https://github.com/DEShawResearch/random123

3. **SFML** — Simple and Fast Multimedia Library.  
   https://www.sfml-dev.org/

