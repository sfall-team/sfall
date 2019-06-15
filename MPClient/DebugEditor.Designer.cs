namespace FalloutClient {
    partial class DebugEditor {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing) {
            if(disposing && (components != null)) {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent() {
            this.dataGridView1 = new System.Windows.Forms.DataGridView();
            this.Column0 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Column1 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Column2 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Column3 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.bGlobals = new System.Windows.Forms.Button();
            this.bMapVars = new System.Windows.Forms.Button();
            this.bCritters = new System.Windows.Forms.Button();
            this.bSGlobals = new System.Windows.Forms.Button();
            this.bArrays = new System.Windows.Forms.Button();
            this.bEdit = new System.Windows.Forms.Button();
            this.bCrittersLvar = new System.Windows.Forms.Button();
            ((System.ComponentModel.ISupportInitialize)(this.dataGridView1)).BeginInit();
            this.SuspendLayout();
            // 
            // dataGridView1
            // 
            this.dataGridView1.AllowUserToAddRows = false;
            this.dataGridView1.AllowUserToDeleteRows = false;
            this.dataGridView1.AllowUserToResizeRows = false;
            this.dataGridView1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.dataGridView1.ColumnHeadersHeight = 22;
            this.dataGridView1.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.Column0,
            this.Column1,
            this.Column2,
            this.Column3});
            this.dataGridView1.Location = new System.Drawing.Point(12, 12);
            this.dataGridView1.MultiSelect = false;
            this.dataGridView1.Name = "dataGridView1";
            this.dataGridView1.RowHeadersVisible = false;
            this.dataGridView1.RowHeadersWidthSizeMode = System.Windows.Forms.DataGridViewRowHeadersWidthSizeMode.DisableResizing;
            this.dataGridView1.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.FullRowSelect;
            this.dataGridView1.Size = new System.Drawing.Size(561, 354);
            this.dataGridView1.TabIndex = 0;
            this.dataGridView1.CellDoubleClick += new System.Windows.Forms.DataGridViewCellEventHandler(this.dataGridView1_CellDoubleClick);
            this.dataGridView1.CellEndEdit += new System.Windows.Forms.DataGridViewCellEventHandler(this.dataGridView1_CellEndEdit);
            this.dataGridView1.RowsAdded += new System.Windows.Forms.DataGridViewRowsAddedEventHandler(this.dataGridView1_RowsAdded);
            // 
            // Column0
            // 
            this.Column0.HeaderText = "ID";
            this.Column0.Name = "Column0";
            this.Column0.ReadOnly = true;
            this.Column0.Width = 50;
            // 
            // Column1
            // 
            this.Column1.AutoSizeMode = System.Windows.Forms.DataGridViewAutoSizeColumnMode.Fill;
            this.Column1.HeaderText = "Name";
            this.Column1.Name = "Column1";
            this.Column1.ReadOnly = true;
            // 
            // Column2
            // 
            this.Column2.HeaderText = "Value (Int)";
            this.Column2.Name = "Column2";
            // 
            // Column3
            // 
            this.Column3.HeaderText = "Value (Float)";
            this.Column3.Name = "Column3";
            // 
            // bGlobals
            // 
            this.bGlobals.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.bGlobals.Location = new System.Drawing.Point(12, 372);
            this.bGlobals.Name = "bGlobals";
            this.bGlobals.Size = new System.Drawing.Size(100, 23);
            this.bGlobals.TabIndex = 1;
            this.bGlobals.Text = "Globals";
            this.bGlobals.UseVisualStyleBackColor = true;
            this.bGlobals.Click += new System.EventHandler(this.bGlobals_Click);
            // 
            // bMapVars
            // 
            this.bMapVars.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.bMapVars.Location = new System.Drawing.Point(118, 372);
            this.bMapVars.Name = "bMapVars";
            this.bMapVars.Size = new System.Drawing.Size(100, 23);
            this.bMapVars.TabIndex = 2;
            this.bMapVars.Text = "Map variables";
            this.bMapVars.UseVisualStyleBackColor = true;
            this.bMapVars.Click += new System.EventHandler(this.bMapVars_Click);
            // 
            // bCritters
            // 
            this.bCritters.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.bCritters.Location = new System.Drawing.Point(12, 401);
            this.bCritters.Name = "bCritters";
            this.bCritters.Size = new System.Drawing.Size(100, 23);
            this.bCritters.TabIndex = 3;
            this.bCritters.Text = "Critters";
            this.bCritters.UseVisualStyleBackColor = true;
            this.bCritters.Click += new System.EventHandler(this.bCritters_Click);
            // 
            // bSGlobals
            // 
            this.bSGlobals.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.bSGlobals.Location = new System.Drawing.Point(367, 372);
            this.bSGlobals.Name = "bSGlobals";
            this.bSGlobals.Size = new System.Drawing.Size(100, 23);
            this.bSGlobals.TabIndex = 4;
            this.bSGlobals.Text = "sfall globals";
            this.bSGlobals.UseVisualStyleBackColor = true;
            this.bSGlobals.Click += new System.EventHandler(this.bSGlobals_Click);
            // 
            // bArrays
            // 
            this.bArrays.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.bArrays.Location = new System.Drawing.Point(473, 372);
            this.bArrays.Name = "bArrays";
            this.bArrays.Size = new System.Drawing.Size(100, 23);
            this.bArrays.TabIndex = 5;
            this.bArrays.Text = "sfall arrays";
            this.bArrays.UseVisualStyleBackColor = true;
            this.bArrays.Click += new System.EventHandler(this.bArrays_Click);
            // 
            // bEdit
            // 
            this.bEdit.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.bEdit.Location = new System.Drawing.Point(473, 401);
            this.bEdit.Name = "bEdit";
            this.bEdit.Size = new System.Drawing.Size(100, 23);
            this.bEdit.TabIndex = 6;
            this.bEdit.Text = "Edit";
            this.bEdit.UseVisualStyleBackColor = true;
            this.bEdit.Click += new System.EventHandler(this.bEdit_Click);
            // 
            // bCrittersLvar
            // 
            this.bCrittersLvar.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.bCrittersLvar.Enabled = false;
            this.bCrittersLvar.Location = new System.Drawing.Point(119, 401);
            this.bCrittersLvar.Name = "bCrittersLvar";
            this.bCrittersLvar.Size = new System.Drawing.Size(99, 23);
            this.bCrittersLvar.TabIndex = 7;
            this.bCrittersLvar.Text = "Local variables";
            this.bCrittersLvar.UseVisualStyleBackColor = true;
            this.bCrittersLvar.Click += new System.EventHandler(this.bCrittersLvar_Click);
            // 
            // DebugEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(585, 436);
            this.Controls.Add(this.bCrittersLvar);
            this.Controls.Add(this.bEdit);
            this.Controls.Add(this.bArrays);
            this.Controls.Add(this.bSGlobals);
            this.Controls.Add(this.bCritters);
            this.Controls.Add(this.bMapVars);
            this.Controls.Add(this.bGlobals);
            this.Controls.Add(this.dataGridView1);
            this.MinimumSize = new System.Drawing.Size(450, 300);
            this.Name = "DebugEditor";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "sfall Debug Editor";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.DebugEditor_FormClosing);
            ((System.ComponentModel.ISupportInitialize)(this.dataGridView1)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.DataGridView dataGridView1;
        private System.Windows.Forms.Button bGlobals;
        private System.Windows.Forms.Button bMapVars;
        private System.Windows.Forms.Button bCritters;
        private System.Windows.Forms.Button bSGlobals;
        private System.Windows.Forms.Button bArrays;
        private System.Windows.Forms.Button bEdit;
        private System.Windows.Forms.DataGridViewTextBoxColumn Column0;
        private System.Windows.Forms.DataGridViewTextBoxColumn Column1;
        private System.Windows.Forms.DataGridViewTextBoxColumn Column2;
        private System.Windows.Forms.DataGridViewTextBoxColumn Column3;
        private System.Windows.Forms.Button bCrittersLvar;

    }
}