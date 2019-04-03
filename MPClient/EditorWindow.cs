using System;
using System.Collections.Generic;
using System.Windows.Forms;

namespace FalloutClient {
    public enum DataType : int { None=0, Int=1, Float=2, String=3 }
    public partial class EditorWindow : Form {
        private readonly DataType[] types;
        private readonly string[] values;
        private bool save;

        private EditorWindow(string[] names, DataType[] types, string[] values) {
            this.types=types;
            this.values=values;
            InitializeComponent();
            dataGridView1.SuspendLayout();
            if(names==null)
                for(int i=0;i<types.Length;i++) dataGridView1.Rows.Add(i.ToString(), types[i].ToString(), values[i]);
            else
                for(int i=0;i<types.Length;i++) dataGridView1.Rows.Add("0x" + (i * 4).ToString("x").ToUpper() + names[i], types[i].ToString(), values[i]);
            dataGridView1.ResumeLayout();
        }

        public static string[] ShowEditor(string[] names, DataType[] types, string[] values) {
            EditorWindow editor = new EditorWindow(names, types, values);
            editor.ShowDialog();
            if (editor.save)
                return editor.values;
            else
                return null;
        }

        private bool CheckInput(DataType type, string str) {
            switch(type) {
            case DataType.Int:
                int i;
                return int.TryParse(str, out i);
            case DataType.Float:
                float f;
                return float.TryParse(str, out f);
            }
            return true;
        }

        private void dataGridView1_CellEndEdit(object sender, DataGridViewCellEventArgs e) {
            string str=(string)dataGridView1.Rows[e.RowIndex].Cells[e.ColumnIndex].Value;
            if(CheckInput(types[e.RowIndex], str)) values[e.RowIndex]=str;
            else dataGridView1.Rows[e.RowIndex].Cells[e.ColumnIndex].Value=values[e.RowIndex];
        }

        private void bCancel_Click(object sender, EventArgs e) {
            Close();
        }

        private void bSave_Click(object sender, EventArgs e) {
            save=true;
            Close();
        }
    }
}
