# Design Report Template (2-4 pages)

## 1. Emulator and Hardware Model Assumptions
- CPU model, pipeline assumptions, cycle model
- Memory model and any fast-memory assumptions
- Power model equations and accounting units

## 2. OS and Runtime Design
- Scheduler type and priorities
- Task periods, deadlines, release model
- Task control representation

## 3. Memory Layout
- Global regions, per-task stacks, static buffers
- Bounds and overflow prevention choices

## 4. Collision and Maneuver Algorithms
- Orbit/debris update model
- Risk classification thresholds
- Maneuver candidate scoring

## 5. Architecture-Aware Optimizations
- Fixed-point math choices
- Hotspot loops and memory locality
- Any assembly kernels and why

## 6. Results
- Recall and precision summary
- Maneuver effectiveness summary
- Cycle and energy usage summary

## 7. Limitations and Future Work
- Current limitations
- Planned improvements
