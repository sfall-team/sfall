using System;
using System.Collections.Generic;
using System.Windows.Forms;

namespace FalloutClient {

    public enum DataType : int { None = 0, Int = 1, Float = 2, String = 3 }

    public partial class EditorWindow : Form
    {
        private readonly DataType[] types = null;
        private string[] values;
        private bool save;
        private static bool valueInHex = false;

        private void ConvertValues(bool toHex) {
            for (int i = 0; i < values.Length; i++) {
                if (toHex) {
                    if (types == null || types[i] == DataType.Int) {
                        int val = int.Parse(values[i]);
                        values[i] = "0x" + val.ToString("x").ToUpper();
                    }
                } else {
                    if (types == null || types[i] == DataType.Int) {
                        values[i] = Convert.ToInt32(values[i], 16).ToString();
                    }
                }
            }
        }

        private EditorWindow(string[] names, DataType[] types, string[] values, bool isMap) {
            this.types = types;
            this.values = values;

            InitializeComponent();
            checkBox.Checked = EditorWindow.valueInHex;
            dataGridView1.SuspendLayout();
            if (valueInHex) ConvertValues(true);
            if (names == null)
                for (int i = 0; i < types.Length; i++) {
                    string element = i.ToString();
                    if (isMap && (i % 2) == 0) element += " - Key";
                    dataGridView1.Rows.Add(element, types[i].ToString(), values[i]);
                }
            else
                for (int i = 0; i < types.Length; i++) dataGridView1.Rows.Add("[0x" + (i * 4).ToString("x").ToUpper() + "]" + names[i], types[i].ToString(), values[i]);
            dataGridView1.ResumeLayout();
        }

        public static string[] ShowEditor(DebugEditor form, string[] names, DataType[] types, string[] values, bool isMap = false) {
            EditorWindow editor = new EditorWindow(names, types, values, isMap);
            editor.ShowDialog();
            if (editor.save) {
                if (valueInHex) editor.ConvertValues(false);
                return editor.values;
            }
            return null;
        }

        private EditorWindow(string[] values) {
            this.values = values;
            InitializeComponent();
            checkBox.Checked = EditorWindow.valueInHex;
            dataGridView1.SuspendLayout();
            if (valueInHex) ConvertValues(true);
            for (int i = 0; i < values.Length; i++) {
                dataGridView1.Rows.Add(i.ToString(), "Int", values[i]);
            }
            dataGridView1.ResumeLayout();
        }

        public static string[] ShowEditor(DebugEditor form, string[] lvalues) {
            EditorWindow editor = new EditorWindow(lvalues);
            editor.ShowDialog();
            if (editor.save) {
                if (valueInHex) editor.ConvertValues(false);
                return editor.values;
            }
            return null;
        }

        private bool CheckInput(DataType type, string str) {
            switch (type) {
            case DataType.Int:
                if (valueInHex) {
                    try {
                        Convert.ToInt32(str, 16);
                        return true;
                    } catch (Exception) {
                        return false;
                    }
                }
                int i;
                return int.TryParse(str, out i);
            case DataType.Float:
                float f;
                return float.TryParse(str, out f);
            }
            return true;
        }

        private void dataGridView1_CellEndEdit(object sender, DataGridViewCellEventArgs e) {
            string str = (string)dataGridView1.Rows[e.RowIndex].Cells[e.ColumnIndex].Value;
            if (str != null && CheckInput(((types != null) ? types[e.RowIndex] : DataType.Int), str))
                values[e.RowIndex] = str;
            else
                dataGridView1.Rows[e.RowIndex].Cells[e.ColumnIndex].Value = values[e.RowIndex];
        }

        private void bCancel_Click(object sender, EventArgs e) {
            Close();
        }

        private void bSave_Click(object sender, EventArgs e) {
            save = true;
            Close();
        }

        private void checkBox_Click(object sender, EventArgs e) {
            valueInHex = checkBox.Checked;
            ConvertValues(valueInHex);
            for (int i = 0; i < values.Length; i++) {
                dataGridView1.Rows[i].Cells[2].Value = values[i];
            }
        }
    }
}
