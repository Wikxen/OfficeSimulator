# Office Simulator

A turn-based office management game for 64-bit Windows 10 and 11. Double-click
`dist\OfficeSimulator.exe` to play. The executable runs offline with no installer,
extra game files, or separately installed runtime.

## How to play

You start alone in an unfurnished Starter Suite with $2,400. Buy furniture,
technology, and amenities, or hire colleagues. Check projected weekly profit
before spending: hiring adds payroll and crowding lowers satisfaction. The type
of work is abstract; happier teams generate more revenue. Profitable weeks
with at least 75% satisfaction improve your reputation, which boosts revenue.

Select **Finish week** whenever you're ready to collect revenue and pay expenses.
Fill your current office before moving to the next tier: Starter Suite, Studio
Office, Team Loft, and Headquarters. Win by keeping a full Headquarters
profitable with at least 85% satisfaction for three consecutive weeks. You can
keep playing after winning.

Hiring adds **6–10 disruption points**, and a move adds **18–24**. Those
penalties stack if you expand quickly: each point takes one point off
satisfaction, down to zero. Completing a week removes **3 disruption points**
after that week's results are calculated. The cohesion meter shows your current
penalty and the weeks left until the team settles if you make no further changes.

The Starter Suite has **1 amenity slot**, the Studio Office has **4**, the Team
Loft has **7**, and Headquarters has **10**. Each perk costs money to install and
maintain. You keep your perks when you move, and older saves keep any perks that
were already installed above the new capacity.

Every upgrade changes the office: desks gain storage and lamps, laptops give
way to dual monitors, and the break area can grow from a coffee station into
a lounge with an arcade cabinet, a library, and more. Watch your team work
between turns and look for short animations when you invest, hire, move, or
finish a week. The game follows the
Windows **Animation effects** setting; turning it off keeps the office still
without changing gameplay.

Use the mouse or **Tab** and **Space**. Each purchase and completed week autosaves
to `%LOCALAPPDATA%\OfficeSimulator\office.save`. **New game** replaces that save
after confirmation.

## Build from source

With x64 MinGW-w64 GCC (such as w64devkit) on `PATH`, run this in PowerShell:

```powershell
.\build.ps1 -Test
```

Omit `-Test` to build just the game. The build always writes the playable
executable to `dist\OfficeSimulator.exe`.
