from __future__ import annotations

import tkinter as tk
from tkinter import filedialog, messagebox
from pathlib import Path

try:
    from .emulator import RiscVEmulator, Snapshot
except ImportError:  # pragma: no cover - direct script execution fallback
    from emulator import RiscVEmulator, Snapshot


TOOLS_DIR = Path(__file__).resolve().parent
PROJECT_DIR = TOOLS_DIR.parent
DEFAULT_ASM = PROJECT_DIR / "asm" / "runtime_kernels_riscv32.s"


class RiscVSimulatorApp:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.root.title("RISC-V Simulator")
        self.root.geometry("1650x850")

        self.emulator: RiscVEmulator | None = None
        self.history: list[Snapshot] = []
        self.stage = 0
        self.source_path = DEFAULT_ASM

        self._build_ui()
        self._load_file(DEFAULT_ASM)

    def _resolve_input_path(self, raw_path: str) -> Path:
        path = Path(raw_path.strip())
        if path.is_absolute():
            return path
        return (PROJECT_DIR / path).resolve()

    def _build_ui(self) -> None:
        self.root.grid_rowconfigure(1, weight=1)
        self.root.grid_columnconfigure(0, weight=1)
        self.root.grid_columnconfigure(1, weight=1)
        self.root.grid_columnconfigure(2, weight=1)
        self.root.grid_columnconfigure(3, weight=1)

        control_frame = tk.Frame(self.root)
        control_frame.grid(row=0, column=0, columnspan=4, sticky="nsew", padx=8, pady=8)
        for index in range(8):
            control_frame.grid_columnconfigure(index, weight=1)

        tk.Label(control_frame, text="Assembly File").grid(row=0, column=0, sticky="w")
        self.file_entry = tk.Entry(control_frame)
        self.file_entry.grid(row=0, column=1, columnspan=4, sticky="nsew", padx=4)
        self.file_entry.insert(0, str(self.source_path))

        tk.Label(control_frame, text="Function").grid(row=0, column=5, sticky="w")
        self.function_entry = tk.Entry(control_frame)
        self.function_entry.grid(row=0, column=6, sticky="nsew", padx=4)
        self.function_entry.insert(0, "rv_scheduler_flags")

        tk.Label(control_frame, text="Args").grid(row=0, column=7, sticky="w")
        self.args_entry = tk.Entry(control_frame)
        self.args_entry.grid(row=0, column=8, sticky="nsew", padx=4)
        self.args_entry.insert(0, "20")

        button_frame = tk.Frame(self.root)
        button_frame.grid(row=1, column=0, columnspan=4, sticky="nsew", padx=8)
        for index in range(6):
            button_frame.grid_columnconfigure(index, weight=1)

        tk.Button(button_frame, text="Open", command=self._open_file).grid(row=0, column=0, sticky="nsew", padx=4, pady=4)
        tk.Button(button_frame, text="Load", command=self._reload_current).grid(row=0, column=1, sticky="nsew", padx=4, pady=4)
        tk.Button(button_frame, text="Step", command=self._step).grid(row=0, column=2, sticky="nsew", padx=4, pady=4)
        tk.Button(button_frame, text="Run", command=self._run).grid(row=0, column=3, sticky="nsew", padx=4, pady=4)
        tk.Button(button_frame, text="Prev", command=self._prev).grid(row=0, column=4, sticky="nsew", padx=4, pady=4)
        tk.Button(button_frame, text="Reset", command=self._reload_current).grid(row=0, column=5, sticky="nsew", padx=4, pady=4)

        panes = tk.Frame(self.root)
        panes.grid(row=2, column=0, columnspan=4, sticky="nsew", padx=8, pady=8)
        panes.grid_rowconfigure(0, weight=1)
        panes.grid_columnconfigure(0, weight=1)
        panes.grid_columnconfigure(1, weight=1)
        panes.grid_columnconfigure(2, weight=1)
        panes.grid_columnconfigure(3, weight=1)

        self.source_text = self._make_text_pane(panes, 0, "Assembly Source", "light yellow")
        self.listing_text = self._make_text_pane(panes, 1, "Instruction Listing", "light green")
        self.state_text = self._make_text_pane(panes, 2, "State and Memory", "light grey")
        self.perf_text = self._make_text_pane(panes, 3, "Performance and Power", "lavender")

        self.status = tk.Label(self.root, text="Ready", anchor="w")
        self.status.grid(row=3, column=0, columnspan=4, sticky="nsew", padx=8, pady=(0, 8))

    def _make_text_pane(self, parent: tk.Widget, column: int, title: str, background: str) -> tk.Text:
        frame = tk.Frame(parent)
        frame.grid(row=0, column=column, sticky="nsew", padx=4)
        frame.grid_rowconfigure(1, weight=1)
        frame.grid_columnconfigure(0, weight=1)

        tk.Label(frame, text=title).grid(row=0, column=0, sticky="nsew")
        text = tk.Text(frame, bg=background, width=40, height=40, borderwidth=3, relief="solid")
        text.grid(row=1, column=0, sticky="nsew")
        scrollbar = tk.Scrollbar(frame, command=text.yview)
        scrollbar.grid(row=1, column=1, sticky="ns")
        text.configure(yscrollcommand=scrollbar.set)
        return text

    def _set_status(self, message: str) -> None:
        self.status.configure(text=message)

    def _render_source(self) -> None:
        content = self.source_path.read_text(encoding="utf-8")
        self.source_text.delete("1.0", tk.END)
        self.source_text.insert(tk.END, content)

    def _render_view(self) -> None:
        if self.emulator is None:
            return
        self.listing_text.delete("1.0", tk.END)
        self.listing_text.insert(tk.END, self.emulator.listing_text())
        self.state_text.delete("1.0", tk.END)
        self.state_text.insert(tk.END, self.emulator.state_text())
        self.perf_text.delete("1.0", tk.END)
        self.perf_text.insert(tk.END, self.emulator.performance_text())

    def _parse_args(self) -> list[int]:
        raw = self.args_entry.get().replace(",", " ").split()
        return [int(item, 0) for item in raw]

    def _load_file(self, path: Path) -> None:
        if not path.exists():
            messagebox.showerror("File not found", f"Unable to find: {path}")
            return
        try:
            self.source_path = path
            self.file_entry.delete(0, tk.END)
            self.file_entry.insert(0, str(path))
            self._render_source()
            self._reload_current()
        except Exception as exc:  # pragma: no cover - GUI feedback path
            messagebox.showerror("Load error", str(exc))

    def _reload_current(self) -> None:
        try:
            path = self._resolve_input_path(self.file_entry.get())
            self.source_path = path
            self.emulator = RiscVEmulator(path)
            requested_function = self.function_entry.get().strip() or "rv_scheduler_flags"
            function_name = requested_function
            if function_name not in self.emulator.program.labels:
                if not self.emulator.program.labels:
                    raise ValueError("No entry labels found in assembly file")
                fallback = "main" if "main" in self.emulator.program.labels else next(iter(self.emulator.program.labels))
                function_name = fallback
                self.function_entry.delete(0, tk.END)
                self.function_entry.insert(0, function_name)
            args = self._parse_args()
            self.emulator.reset(function_name, args)
            self.history = [self.emulator.snapshot()]
            self.stage = 0
            self._render_source()
            self._render_view()
            if function_name != requested_function:
                self._set_status(f"Loaded {path} | entry={function_name} (auto-selected)")
            else:
                self._set_status(f"Loaded {path} | entry={function_name}")
        except Exception as exc:  # pragma: no cover - GUI feedback path
            messagebox.showerror("Load error", str(exc))
            self._set_status("Load failed")

    def _open_file(self) -> None:
        chosen = filedialog.askopenfilename(
            title="Select a RISC-V assembly file",
            filetypes=[("Assembly files", "*.s *.S *.asm"), ("All files", "*.*")],
        )
        if chosen:
            self._load_file(Path(chosen))

    def _ensure_emulator(self) -> bool:
        if self.emulator is None:
            messagebox.showwarning("No program", "Load an assembly file first.")
            return False
        return True

    def _step(self) -> None:
        if not self._ensure_emulator():
            return
        try:
            if self.stage < len(self.history) - 1:
                self.stage += 1
                self.emulator.restore(self.history[self.stage])
            else:
                self.emulator.step()
                self.history.append(self.emulator.snapshot())
                self.stage += 1
            self._render_view()
            self._set_status(f"Step {self.stage}")
        except Exception as exc:  # pragma: no cover - GUI feedback path
            messagebox.showerror("Step error", str(exc))

    def _run(self) -> None:
        if not self._ensure_emulator():
            return
        try:
            while not self.emulator.halted:
                self.emulator.step()
                self.history.append(self.emulator.snapshot())
                self.stage += 1
            self._render_view()
            self._set_status(f"Finished after {self.stage} steps")
        except Exception as exc:  # pragma: no cover - GUI feedback path
            messagebox.showerror("Run error", str(exc))

    def _prev(self) -> None:
        if not self._ensure_emulator():
            return
        try:
            if self.stage <= 0:
                raise ValueError("Cannot go back from the initial stage")
            self.stage -= 1
            self.emulator.restore(self.history[self.stage])
            self._render_view()
            self._set_status(f"Rewound to step {self.stage}")
        except Exception as exc:  # pragma: no cover - GUI feedback path
            messagebox.showerror("Previous state error", str(exc))


def main() -> int:
    root = tk.Tk()
    app = RiscVSimulatorApp(root)
    root.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
