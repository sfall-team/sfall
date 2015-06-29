using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using File=System.IO.File;
using StreamReader=System.IO.StreamReader;
using MemoryStream=System.IO.MemoryStream;
using BinaryReader=System.IO.BinaryReader;
using BinaryWriter=System.IO.BinaryWriter;

namespace FalloutClient {
    public partial class DebugEditor : Form {
        [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Explicit)]
        private struct ByteConverter {
            [System.Runtime.InteropServices.FieldOffset(0)]
            private int i;
            [System.Runtime.InteropServices.FieldOffset(0)]
            private float f;

            public int GetAsInt(float f) {
                this.f=f;
                return i;
            }
            public float GetAsFloat(int i) {
                this.i=i;
                return f;
            }
            public byte[] GetAsBytes(ulong ul) {
                byte[] b=new byte[8];
                for(int i=0;i<8;i++) b[i]=(byte)((ul&((ulong)0xff<<(i*8)))>>(i*8));
                return b;
            }
        }

        private static ByteConverter converter=new ByteConverter();

        private enum Mode { Globals, MapVars, SGlobals, Arrays, Critters }

        private readonly EditorConnection connection;
        private Mode mode;

        private readonly Dictionary<int, string> GlobNames=new Dictionary<int, string>();
        private readonly Dictionary<uint, string> CritNames=new Dictionary<uint, string>();

        private void Redraw() {
            dataGridView1.SuspendLayout();
            dataGridView1.Rows.Clear();
            switch(mode) {
            case Mode.Globals:
                Column2.ReadOnly=false;
                Column3.ReadOnly=false;
                Column2.HeaderText="Value (Int)";
                Column3.HeaderText="Value (Float)";
                bEdit.Enabled=false;
                for(int i=0;i<connection.Globals.Length;i++) {
                    string name=GlobNames.ContainsKey(i)?GlobNames[i]:"";
                    dataGridView1.Rows.Add(i, name, connection.Globals[i], converter.GetAsFloat(connection.Globals[i]));
                }
                break;
            case Mode.MapVars:
                Column2.ReadOnly=false;
                Column3.ReadOnly=false;
                Column2.HeaderText="Value (Int)";
                Column3.HeaderText="Value (Float)";
                bEdit.Enabled=false;
                for(int i=0;i<connection.MapVars.Length;i++) {
                    dataGridView1.Rows.Add(i, "", connection.MapVars[i], converter.GetAsFloat(connection.MapVars[i]));
                }
                break;
            case Mode.SGlobals:
                Column2.ReadOnly=false;
                Column3.ReadOnly=false;
                Column2.HeaderText="Value (Int)";
                Column3.HeaderText="Value (Float)";
                bEdit.Enabled=false;
                for(int i=0;i<connection.SGlobalKeys.Length;i++) {
                    string s=connection.SGlobalKeys[i]>0xffffffff?new string(System.Text.ASCIIEncoding.ASCII.GetChars(converter.GetAsBytes(connection.SGlobalKeys[i]))):("0x"+connection.SGlobalKeys[i].ToString("x"));
                    dataGridView1.Rows.Add(s, "", connection.sGlobals[i], converter.GetAsFloat(connection.sGlobals[i]));
                }
                break;
            case Mode.Arrays:
                Column2.ReadOnly=true;
                Column3.ReadOnly=true;
                Column2.HeaderText="Array length";
                Column3.HeaderText="Element size";
                bEdit.Enabled=true;
                for(int i=0;i<connection.Arrays.Length;i++) {
                    dataGridView1.Rows.Add(connection.Arrays[i], "", connection.ArrayLengths[i], connection.ArrayDataSizes[i]);
                }
                break;
            case Mode.Critters:
                Column2.ReadOnly=true;
                Column3.ReadOnly=true;
                Column2.HeaderText="pid";
                Column3.HeaderText="";
                bEdit.Enabled=true;
                for(int i=0;i<connection.Critters.Length;i++) {
                    uint modcrit=(connection.Critters[i]&0xfffff);
                    string name=CritNames.ContainsKey(modcrit)?CritNames[modcrit]:"";
                    dataGridView1.Rows.Add(i+1, name, connection.Critters[i].ToString("x"), "");
                }
                break;
            }
            dataGridView1.ResumeLayout();
        }

        internal DebugEditor(EditorConnection connection) {
            InitializeComponent();
            this.connection=connection;
            mode=Mode.Globals;
            if(File.Exists("globals.txt")) {
                StreamReader sr=new StreamReader("globals.txt");
                while(!sr.EndOfStream) {
                    string[] line=sr.ReadLine().Split(' ');
                    GlobNames[int.Parse(line[0])]=line[1];
                }
                sr.Close();
            }
            if(File.Exists("critters.txt")) {
                StreamReader sr=new StreamReader("critters.txt");
                while(!sr.EndOfStream) {
                    string line=sr.ReadLine();
                    CritNames[uint.Parse(line.Remove(line.IndexOf(' ')))]=line.Substring(line.IndexOf(' ')+1);
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
            mode=Mode.Globals;
            Redraw();
        }

        private void bMapVars_Click(object sender, EventArgs e) {
            mode=Mode.MapVars;
            Redraw();
        }

        private void bSGlobals_Click(object sender, EventArgs e) {
            mode=Mode.SGlobals;
            Redraw();
        }

        private void bArrays_Click(object sender, EventArgs e) {
            mode=Mode.Arrays;
            Redraw();
        }

        private void bCritters_Click(object sender, EventArgs e) {
            mode=Mode.Critters;
            Redraw();
        }

        private void dataGridView1_CellEndEdit(object sender, DataGridViewCellEventArgs e) {
            int[] array=null;
            DataTypeSend dts;
            switch(mode) {
            case Mode.Globals:
                array=connection.Globals;
                dts=DataTypeSend.SetGlobal;
                break;
            case Mode.MapVars:
                array=connection.MapVars;
                dts=DataTypeSend.SetMapVar;
                break;
            case Mode.SGlobals:
                array=connection.sGlobals;
                dts=DataTypeSend.SetSGlobal;
                break;
            default:
                return;
            }
            if(e.ColumnIndex==2) {
                int val;
                if(!int.TryParse(dataGridView1.Rows[e.RowIndex].Cells[e.ColumnIndex].Value.ToString(), out val)) {
                    dataGridView1.Rows[e.RowIndex].Cells[e.ColumnIndex].Value=array[e.RowIndex];
                } else {
                    array[e.RowIndex]=val;
                    connection.WriteDataType(dts);
                    connection.WriteInt(e.RowIndex);
                    connection.WriteInt(val);
                }
            } else {
                float val;
                if(!float.TryParse(dataGridView1.Rows[e.RowIndex].Cells[e.ColumnIndex].Value.ToString(), out val)) {
                    dataGridView1.Rows[e.RowIndex].Cells[e.ColumnIndex].Value=array[e.RowIndex];
                } else {
                    array[e.RowIndex]=converter.GetAsInt(val);
                    connection.WriteDataType(dts);
                    connection.WriteInt(e.RowIndex);
                    connection.WriteInt(converter.GetAsInt(val));
                }
            }
        }

        private void bEdit_Click(object sender, EventArgs e) {
            if(dataGridView1.SelectedRows.Count!=1) return;
            int i=dataGridView1.SelectedRows[0].Index;
            switch(mode) {
            case Mode.Arrays: {
                    DataType[] types=new DataType[connection.ArrayLengths[i]];
                    string[] strings=new string[connection.ArrayLengths[i]];
                    connection.WriteDataType(DataTypeSend.GetArray);
                    connection.WriteInt(i);
                    for(int j=0;j<strings.Length;j++) types[j]=(DataType)connection.ReadInt();
                    byte[] buf=connection.ReadBytes(connection.ArrayLengths[i]*connection.ArrayDataSizes[i]);
                    MemoryStream ms=new MemoryStream(buf);
                    BinaryReader br=new BinaryReader(ms);
                    for(int j=0;j<strings.Length;j++) {
                        ms.Position=j*connection.ArrayDataSizes[i];
                        switch(types[j]) {
                        case DataType.Int:
                            strings[j]=br.ReadInt32().ToString();
                            break;
                        case DataType.Float:
                            strings[j]=br.ReadSingle().ToString();
                            break;
                        case DataType.String:
                            byte[] bytes=br.ReadBytes(connection.ArrayDataSizes[i]);
                            strings[j]=System.Text.Encoding.ASCII.GetString(bytes, 0, Array.IndexOf<byte>(bytes, 0));
                            break;
                        }
                    }
                    br.Close();
                    strings=EditorWindow.ShowEditor(null, types, strings);
                    if(strings!=null) {
                        connection.WriteDataType(DataTypeSend.SetArray);
                        connection.WriteInt(i);
                        ms=new MemoryStream(connection.ArrayLengths[i]*connection.ArrayDataSizes[i]);
                        BinaryWriter bw=new BinaryWriter(ms);
                        for(int j=0;j<strings.Length;j++) {
                            ms.Position=j*connection.ArrayDataSizes[i];
                            switch(types[j]) {
                            case DataType.Int:
                                bw.Write(int.Parse(strings[j]));
                                break;
                            case DataType.Float:
                                bw.Write(float.Parse(strings[j]));
                                break;
                            case DataType.String:
                                byte[] bytes=System.Text.Encoding.ASCII.GetBytes(strings[j]);
                                if(bytes.Length<connection.ArrayDataSizes[i]) bw.Write(bytes);
                                else bw.Write(bytes, 0, connection.ArrayDataSizes[i]-1);
                                bw.Write(0);
                                break;
                            }
                        }
                        connection.WriteBytes(ms.GetBuffer(), 0, connection.ArrayLengths[i]*connection.ArrayDataSizes[i]);
                        bw.Close();
                    }
                }
                break;
            case Mode.Critters: {
                    DataType[] types=new DataType[29];
                    string[] strings=new string[29];
                    string[] names=new string[29];
                    connection.WriteDataType(DataTypeSend.RetrieveCritter);
                    connection.WriteInt(i);
                    BinaryReader br=new BinaryReader(new System.IO.MemoryStream(connection.ReadBytes(29*4)));
                    for(int j=0;j<29;j++) {
                        types[j]=DataType.Int;
                        strings[j]=br.ReadInt32().ToString();
                        names[j]="0x"+(j*4).ToString("x");
                    }
                    br.Close();
                    names[1]="Tile";
                    names[10]="Elevation";
                    names[11]="Inventory count";
                    names[13]="Inventory pointer";
                    names[16]="Current AP";
                    names[17]="Crippled limbs";
                    names[22]="HP";
                    names[23]="Rads";
                    names[24]="Poison";
                    names[25]="Proto ID";
                    strings=EditorWindow.ShowEditor(names, types, strings);
                    if(strings!=null) {
                        MemoryStream ms=new MemoryStream(29*4);
                        BinaryWriter bw=new BinaryWriter(ms);
                        for(int j=0;j<29;j++) bw.Write(int.Parse(strings[j]));
                        connection.WriteDataType(DataTypeSend.SetCritter);
                        connection.WriteInt(i);
                        connection.WriteBytes(ms.GetBuffer(), 0, 29*4);
                        bw.Close();
                    }

                }
                break;
            }
        }
    }
}
