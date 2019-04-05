using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using File = System.IO.File;
using StreamReader = System.IO.StreamReader;
using MemoryStream = System.IO.MemoryStream;
using BinaryReader = System.IO.BinaryReader;
using BinaryWriter = System.IO.BinaryWriter;

namespace FalloutClient {

    public partial class DebugEditor : Form
    {
        [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Explicit)]
        private struct ByteConverter {
            [System.Runtime.InteropServices.FieldOffset(0)]
            private int i;
            [System.Runtime.InteropServices.FieldOffset(0)]
            private float f;

            public int GetAsInt(float f) {
                this.f = f;
                return i;
            }
            public float GetAsFloat(int i) {
                this.i = i;
                return f;
            }
            public byte[] GetAsBytes(ulong ul) {
                byte[] b = new byte[8];
                for (int i = 0; i < 8; i++) b[i] = (byte)((ul & ((ulong)0xff << (i * 8))) >> (i * 8));
                return b;
            }
        }

        private static ByteConverter converter = new ByteConverter();

        private enum Mode { Globals, MapVars, SGlobals, Arrays, Critters }

        private readonly EditorConnection connection;
        private Mode mode;

        private readonly Dictionary<int, string> GlobNames = new Dictionary<int, string>();
        private readonly Dictionary<uint, string> CritNames = new Dictionary<uint, string>();

        private void Redraw() {
            bEdit.Enabled = false;
            Column2.ReadOnly = false;
            Column3.ReadOnly = false;
            Column2.HeaderText = "Value (Int)";
            Column3.HeaderText = "Value (Float)";
            dataGridView1.SelectionMode = DataGridViewSelectionMode.CellSelect;
            dataGridView1.SuspendLayout();
            dataGridView1.Rows.Clear();
            switch (mode) {
            case Mode.Globals:
                for (int i = 0; i < connection.Globals.Length; i++) {
                    string name = GlobNames.ContainsKey(i) ? GlobNames[i] : "";
                    dataGridView1.Rows.Add(i, name, connection.Globals[i], converter.GetAsFloat(connection.Globals[i]));
                }
                break;
            case Mode.MapVars:
                for (int i = 0; i < connection.MapVars.Length; i++) {
                    dataGridView1.Rows.Add(i, "", connection.MapVars[i], converter.GetAsFloat(connection.MapVars[i]));
                }
                break;
            case Mode.SGlobals:
                for (int i = 0; i < connection.SGlobalKeys.Length; i++) {
                    string s = connection.SGlobalKeys[i] > 0xffffffff
                        ? new string(System.Text.ASCIIEncoding.ASCII.GetChars(converter.GetAsBytes(connection.SGlobalKeys[i])))
                        : (connection.SGlobalKeys[i].ToString());
                    dataGridView1.Rows.Add(s, "", connection.sGlobals[i], converter.GetAsFloat(connection.sGlobals[i]));
                }
                break;
            case Mode.Arrays:
                dataGridView1.SelectionMode = DataGridViewSelectionMode.FullRowSelect;
                Column2.ReadOnly = true;
                Column3.ReadOnly = true;
                Column2.HeaderText = "Array size";
                Column3.HeaderText = "Array flags";
                bEdit.Enabled = true;
                for (int i = 0; i < connection.Arrays.Length; i++) {
                    bool isMap = connection.ArrayIsMap[i];
                    int  arrlen = connection.ArrayLengths[i];
                    if (isMap) arrlen /= 2;
                    dataGridView1.Rows.Add(connection.Arrays[i], isMap ? "Map" : "", arrlen, "0x" + connection.ArrayFlag[i].ToString("x").ToUpper());
                }
                break;
            case Mode.Critters:
                dataGridView1.SelectionMode = DataGridViewSelectionMode.FullRowSelect;
                Column2.ReadOnly = true;
                Column3.ReadOnly = true;
                Column2.HeaderText = "PID";
                Column3.HeaderText = "Pointer";
                bEdit.Enabled = true;
                for (int i = 0; i < connection.Critters.Length / 2; i++) {
                    uint modcrit = (connection.Critters[i, 0] & 0xfffff);
                    string name = CritNames.ContainsKey(modcrit) ? CritNames[modcrit] : "";
                    dataGridView1.Rows.Add(i + 1, name, connection.Critters[i, 0].ToString("x").ToUpper(), connection.Critters[i, 1].ToString());
                }
                break;
            }
            dataGridView1.ResumeLayout();
        }

        internal DebugEditor(EditorConnection connection) {
            InitializeComponent();
            this.connection = connection;
            mode = Mode.Globals;
            if (File.Exists(".\\data\\data\\vault13.gam")) {
                StreamReader sr=new StreamReader(".\\data\\data\\vault13.gam");
                int gvar = 0;
                while (!sr.EndOfStream) {
                    string line = sr.ReadLine().TrimStart();
                    if (line.StartsWith("GVAR_")) GlobNames[gvar++] = line.Remove(line.IndexOf(':')).TrimEnd();
                }
                sr.Close();
            } else if (File.Exists("globals.txt")) {
                StreamReader sr = new StreamReader("globals.txt");
                while (!sr.EndOfStream) {
                    string[] line = sr.ReadLine().Split(' ');
                    GlobNames[int.Parse(line[0])] = line[1];
                }
                sr.Close();
            }
            if (File.Exists("critters.txt")) {
                StreamReader sr = new StreamReader("critters.txt");
                while (!sr.EndOfStream) {
                    string line = sr.ReadLine();
                    CritNames[uint.Parse(line.Remove(line.IndexOf(' ')))] = line.Substring(line.IndexOf(' ') + 1);
                }
                sr.Close();
            }
            Redraw();
        }

        private void DebugEditor_FormClosing(object sender, FormClosingEventArgs e) {
            connection.WriteDataType(DataTypeSend.Exit);
            connection.Close();
        }

        private void bGlobals_Click(object sender, EventArgs e) {
            mode = Mode.Globals;
            Redraw();
        }

        private void bMapVars_Click(object sender, EventArgs e) {
            mode = Mode.MapVars;
            Redraw();
        }

        private void bSGlobals_Click(object sender, EventArgs e) {
            mode = Mode.SGlobals;
            Redraw();
        }

        private void bArrays_Click(object sender, EventArgs e) {
            mode = Mode.Arrays;
            Redraw();
        }

        private void bCritters_Click(object sender, EventArgs e) {
            mode = Mode.Critters;
            Redraw();
        }

        private void dataGridView1_CellEndEdit(object sender, DataGridViewCellEventArgs e) {
            int[] array = null;
            DataTypeSend dts;
            switch (mode) {
            case Mode.Globals:
                array = connection.Globals;
                dts = DataTypeSend.SetGlobal;
                break;
            case Mode.MapVars:
                array = connection.MapVars;
                dts = DataTypeSend.SetMapVar;
                break;
            case Mode.SGlobals:
                array = connection.sGlobals;
                dts = DataTypeSend.SetSGlobal;
                break;
            default:
                return;
            }
            if (e.ColumnIndex == 2) {
                string str = dataGridView1.Rows[e.RowIndex].Cells[e.ColumnIndex].Value.ToString();
                int val;
                if (str == null || !int.TryParse(str, out val)) {
                    dataGridView1.Rows[e.RowIndex].Cells[e.ColumnIndex].Value = array[e.RowIndex];
                } else {
                    array[e.RowIndex] = val;
                    connection.WriteDataType(dts);
                    connection.WriteInt(e.RowIndex);
                    connection.WriteInt(val);
                }
            } else {
                string str = dataGridView1.Rows[e.RowIndex].Cells[e.ColumnIndex].Value.ToString();
                float val;
                if (str == null || !float.TryParse(str, out val)) {
                    dataGridView1.Rows[e.RowIndex].Cells[e.ColumnIndex].Value = array[e.RowIndex];
                } else {
                    array[e.RowIndex] = converter.GetAsInt(val);
                    connection.WriteDataType(dts);
                    connection.WriteInt(e.RowIndex);
                    connection.WriteInt(converter.GetAsInt(val));
                }
            }
        }

        private void bEdit_Click(object sender, EventArgs e) {
            if (dataGridView1.SelectedRows.Count == 0) return;
            int i = (int)dataGridView1.SelectedRows[0].Tag;
            switch (mode) {
            case Mode.Arrays: {
                    DataType[] types = new DataType[connection.ArrayLengths[i]];
                    int[] lenType = new int[connection.ArrayLengths[i]];
                    string[] strings = new string[connection.ArrayLengths[i]];
                    connection.WriteDataType(DataTypeSend.GetArray); // code
                    connection.WriteInt(i); // index
                    int lenData = 0;
                    for (int j = 0; j < strings.Length; j++) {
                        types[j] = (DataType)connection.ReadInt();
                        lenType[j] = connection.ReadInt(); // len data of type
                        lenData += lenType[j];
                    }
                    byte[] buf = connection.ReadBytes(lenData); // read data
                    BinaryReader br = new BinaryReader(new MemoryStream(buf));
                    for (int j = 0; j < strings.Length; j++) {
                        switch (types[j]) {
                        case DataType.Int:
                            strings[j] = br.ReadInt32().ToString();
                            break;
                        case DataType.Float:
                            strings[j] = br.ReadSingle().ToString();
                            break;
                        case DataType.String:
                            byte[] bytes = br.ReadBytes(lenType[j]);
                            strings[j] = System.Text.Encoding.ASCII.GetString(bytes, 0, Array.IndexOf<byte>(bytes, 0));
                            break;
                        }
                    }
                    br.Close();
                    strings = EditorWindow.ShowEditor(this, null, types, strings, connection.ArrayIsMap[i]);
                    if (strings != null) { // save
                        MemoryStream ms = new MemoryStream(lenData);
                        BinaryWriter bw = new BinaryWriter(ms);
                        for (int j = 0; j < strings.Length; j++) {
                            switch (types[j]) {
                            case DataType.Int:
                                bw.Write(int.Parse(strings[j]));
                                break;
                            case DataType.Float:
                                bw.Write(float.Parse(strings[j]));
                                break;
                            case DataType.String:
                                byte[] bytes = System.Text.Encoding.ASCII.GetBytes(strings[j]);
                                if (bytes.Length < lenType[j])
                                    bw.Write(bytes);
                                else
                                    bw.Write(bytes, 0, lenType[j] - 1);
                                bw.Write((byte)0);
                                break;
                            }
                        }
                        // send data to sfall
                        connection.WriteDataType(DataTypeSend.SetArray);
                        connection.WriteInt(i); // index
                        connection.WriteInt(lenData);
                        connection.WriteBytes(ms.GetBuffer(), 0, lenData);
                        bw.Close();
                    }
                }
                break;
            case Mode.Critters: {
                    DataType[] types = new DataType[33];
                    string[] strings = new string[33];
                    string[] names = new string[33];
                    connection.WriteDataType(DataTypeSend.RetrieveCritter);
                    connection.WriteInt(i);
                    BinaryReader br = new BinaryReader(new System.IO.MemoryStream(connection.ReadBytes(33 * 4)));
                    for (int j = 0; j < 33; j++) {
                        types[j] = DataType.Int;
                        strings[j] = br.ReadInt32().ToString();
                    }
                    br.Close();
                    names[0] = " ID";
                    names[1] = " Tile";
                    names[6] = " Current frame";
                    names[7] = " Rotation";
                    names[8] = " FID";
                    names[9] = " Flags";
                    names[10] = " Elevation";
                    names[11] = " Inventory count";
                    names[13] = " Inventory pointer";
                    names[14] = " Reaction";
                    names[15] = " Combat state";
                    names[16] = " Current AP";
                    names[17] = " Combat flags";
                    names[18] = " Last Turn Damage";
                    names[19] = " AI Packet";
                    names[20] = " Team";
                    names[21] = " Who hit me";
                    names[22] = " HP";
                    names[23] = " Rads";
                    names[24] = " Poison";
                    names[25] = " Proto ID";
                    names[26] = " Combat ID";
                    names[29] = " Outline flags";
                    names[30] = " Script ID";
                    names[32] = " Script index";
                    strings = EditorWindow.ShowEditor(this, names, types, strings);
                    if (strings != null) {
                        MemoryStream ms = new MemoryStream(33 * 4);
                        BinaryWriter bw = new BinaryWriter(ms);
                        for (int j = 0; j < 33; j++) bw.Write(int.Parse(strings[j]));
                        connection.WriteDataType(DataTypeSend.SetCritter);
                        connection.WriteInt(i);
                        connection.WriteBytes(ms.GetBuffer(), 0, 33 * 4);
                        bw.Close();
                    }
                }
                break;
            }
        }

        private void dataGridView1_CellDoubleClick(object sender, DataGridViewCellEventArgs e) {
            bEdit.PerformClick();
        }

        private void dataGridView1_RowsAdded(object sender, DataGridViewRowsAddedEventArgs e) {
            dataGridView1.Rows[e.RowIndex].Tag = e.RowIndex;
        }
    }
}
