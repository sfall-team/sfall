using System;
using System.Windows.Forms;
using System.IO;

namespace FalloutClient {
    static class Program {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main(string[] args) {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            if (args.Length == 1 && args[0] == "-debugedit") {
                Application.Run(new DebugEditor(new EditorConnection()));
            } else {
                MessageBox.Show("The debugger can be run only from the game.",
                    "sfall debug editor", MessageBoxButtons.OK, MessageBoxIcon.Stop);
            }
        }
    }

    enum DataTypeSend {
        SetGlobal = 0,
        SetMapVar = 1,
        RetrieveCritter = 2,
        SetCritter = 3,
        SetSGlobal = 4,
        GetArray = 9,
        SetArray = 10,
        Exit = 254,
        RedrawGame = 255
    }
}
