# Map Layout

## Coordinate Grid

Columns (X): `-12`, `-8`, `-4`, `0`, `4`, `8`, `12`
Rows (Z): `8`, `5`, `0`, `-5`, `-8`

Special node at (0,0): C4 splits into **C4EN** (z=+2) and **C4ES** (z=-2), connected by a short vertical segment.

Legend:
- `[XX]` — named road checkpoint node
- `[XX*]` — exam checkpoint (marked with `*`)
- `--` / `|` — road segment (ST)
- `<G>` — gateway node (GA), off-grid entry/exit

---

## Grid Map

```
                     X: -12       -8       -4        0        4        8       12
                          |        |        |        |        |        |        |
         <G9>                                                                           <G11>
          |                                                                               |
Z= 8 :  <G1>--[A1]----[A2]----[A3]----[A4]----[A5]----[A6]----[A7]--<G2>
                 |       |       |       |       |       |       |
Z= 5 :         [B1]----[B2]----[B3]----[B4]----[B5]----[B6]----[B7]
                 |       |       |       |       |       |       |
                 |       |       |     [C4EN]    |       |       |
Z= 0 : <G7>--[C1]----[C2]----[C3]      |     [C5]----[C6]----[C7]--<G8>
                 |       |       |     [C4ES]    |       |       |
                 |       |       |       |       |       |       |
Z=-5 : <G12>--[D1]----[D2]----[D3]----[D4]----[D5]----[D6]----[D7]--<G10>
                 |       |       |       |       |       |       |
Z=-8 :  <G4>--[E1]----[E2]----[E3]----[E4]----[E5]----[E6]----[E7]--<G5>
                                                                         |
                                                                        <G6>
```

> Note: C3 and C5 have no horizontal segment between them (the intersection at x=0,z=0 is handled
> by the split nodes C4EN/C4ES). Road row C is broken: `C1--C2--C3` ... gap ... `C5--C6--C7`.

---

## Named Nodes (all coordinates)

| Node   | Type | X    | Y    | Z    |
|--------|------|------|------|------|
| A1     | CR   | -12  | 0    | 8    |
| A2     | CR   | -8   | 0    | 8    |
| A3     | CR   | -4   | 0    | 8    |
| A4     | CL   |  0   | 0    | 8    |
| A5     | CR   |  4   | 0    | 8    |
| A6     | CR   |  8   | 0    | 8    |
| A7     | CR   |  12  | 0    | 8    |
| B1     | CL   | -12  | 0    | 5    |
| B2     | CL   | -8   | 0    | 5    |
| B3     | CL   | -4   | 0    | 5    |
| B4     | CL   |  0   | 0    | 5    |
| B5     | CL   |  4   | 0    | 5    |
| B6     | CL   |  8   | 0    | 5    |
| B7     | CL   |  12  | 0    | 5    |
| C1     | CL   | -12  | 0    | 0    |
| C2     | CL   | -8   | 0    | 0    |
| C3     | CL   | -4   | 0    | 0    |
| C4EN   | CR   |  0   | 0.55 | +2   |
| C4ES   | CR   |  0   | 0.55 | -2   |
| C5     | CL   |  4   | 0    | 0    |
| C6     | CL   |  8   | 0    | 0    |
| C7     | CL   |  12  | 0    | 0    |
| D1     | CL   | -12  | 0    | -5   |
| D2     | CL   | -8   | 0    | -5   |
| D3     | CL   | -4   | 0    | -5   |
| D4     | CL   |  0   | 0    | -5   |
| D5     | CL   |  4   | 0    | -5   |
| D6     | CL   |  8   | 0    | -5   |
| D7     | CL   |  12  | 0    | -5   |
| E1     | CR   | -12  | 0    | -8   |
| E2     | CR   | -8   | 0    | -8   |
| E3     | CR   | -4   | 0    | -8   |
| E4     | CL   |  0   | 0    | -8   |
| E5     | CR   |  4   | 0    | -8   |
| E6     | CR   |  8   | 0    | -8   |
| E7     | CR   |  12  | 0    | -8   |

---

## Gateway Nodes (off-grid entry/exit)

| Gate | Side | Connected To | Position        | Width | Speed |
|------|------|--------------|-----------------|-------|-------|
| G1   | C    | A1           | -16, 0, 8       | 5     | 20    |
| G2   | C    | A7           | +16, 0, 8       | 5     | 20    |
| G3   | B    | A4           |   0, 0, +12     | 6     | 15    |
| G4   | C    | E1           | -16, 0, -8      | 5     | 20    |
| G5   | C    | E7           | +16, 0, -8      | 5     | 20    |
| G6   | B    | E4           |   0, 0, -12     | 6     | 15    |
| G7   | C    | C1           | -16, 0, 0       | 4     | 25    |
| G8   | C    | C7           | +16, 0, 0       | 4     | 25    |
| G9   | K    | B1           | -16, 0, 5       | 5     | 20    |
| G10  | K    | D7           | +16, 0, -5      | 5     | 20    |
| G11  | B    | B7           | +16, 0, 5       | 6     | 15    |
| G12  | C    | D1           | -16, 0, -5      | 5     | 20    |

---

## Exam Checkpoints (`exampleExam.txt`)

| # | Position (X, Y, Z) | Task                  | Node nearest |
|---|--------------------|-----------------------|--------------|
| 1 | -4, 0, 8           | Navigate_Intersection | **A3**       |
| 2 |  4, 0, -5          | Stop_at_Red_Light     | **D5**       |
| 3 |  8, 0, 8           | Park_Safely           | **A6**       |

### Checkpoints marked on grid

```
                     X: -12       -8       -4        0        4        8       12
Z= 8 :  <G1>--[A1]----[A2]---[A3*1]---[A4]----[A5]---[A6*3]---[A7]--<G2>
                 |       |       |       |       |       |       |
Z= 5 :         [B1]----[B2]----[B3]----[B4]----[B5]----[B6]----[B7]
                 |       |       |     [C4EN]    |       |       |
Z= 0 : <G7>--[C1]----[C2]----[C3]      |     [C5]----[C6]----[C7]--<G8>
                 |       |       |     [C4ES]    |       |       |
Z=-5 : <G12>--[D1]----[D2]----[D3]----[D4]---[D5*2]---[D6]----[D7]--<G10>
                 |       |       |       |       |       |       |
Z=-8 :  <G4>--[E1]----[E2]----[E3]----[E4]----[E5]----[E6]----[E7]--<G5>
```

`*1` = Navigate_Intersection  
`*2` = Stop_at_Red_Light  
`*3` = Park_Safely  

---

## Exam Parameters

| Parameter   | Value |
|-------------|-------|
| Time Limit  | 180 s |
| Pass Score  | 70    |
| Start Score | 100   |
