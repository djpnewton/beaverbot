namespace kowhai_test
{
    partial class MainForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.btnRefreshTrees = new System.Windows.Forms.Button();
            this.btnSave = new System.Windows.Forms.Button();
            this.btnLoad = new System.Windows.Forms.Button();
            this.btnMerge = new System.Windows.Forms.Button();
            this.rbSettings = new System.Windows.Forms.RadioButton();
            this.kowhaiTreeSettings = new kowhai_sharp.KowhaiTree();
            this.btnDiff = new System.Windows.Forms.Button();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.panel1 = new System.Windows.Forms.Panel();
            this.trackBar2 = new System.Windows.Forms.TrackBar();
            this.trackBar1 = new System.Windows.Forms.TrackBar();
            this.pnlScope = new System.Windows.Forms.Panel();
            this.kowhaiTreeScratch = new kowhai_sharp.KowhaiTree();
            this.label1 = new System.Windows.Forms.Label();
            this.btnLoadScratch = new System.Windows.Forms.Button();
            this.hid = new UsbLibrary.UsbHidPort(this.components);
            this.tableLayoutPanel1.SuspendLayout();
            this.panel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar2)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar1)).BeginInit();
            this.SuspendLayout();
            // 
            // btnRefreshTrees
            // 
            this.btnRefreshTrees.Location = new System.Drawing.Point(12, 11);
            this.btnRefreshTrees.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.btnRefreshTrees.Name = "btnRefreshTrees";
            this.btnRefreshTrees.Size = new System.Drawing.Size(116, 26);
            this.btnRefreshTrees.TabIndex = 0;
            this.btnRefreshTrees.Text = "Refresh Trees";
            this.btnRefreshTrees.UseVisualStyleBackColor = true;
            this.btnRefreshTrees.Click += new System.EventHandler(this.btnRefreshTrees_Click);
            // 
            // btnSave
            // 
            this.btnSave.Location = new System.Drawing.Point(157, 11);
            this.btnSave.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.btnSave.Name = "btnSave";
            this.btnSave.Size = new System.Drawing.Size(75, 26);
            this.btnSave.TabIndex = 6;
            this.btnSave.Text = "Save";
            this.btnSave.UseVisualStyleBackColor = true;
            this.btnSave.Click += new System.EventHandler(this.btnSave_Click);
            // 
            // btnLoad
            // 
            this.btnLoad.Location = new System.Drawing.Point(237, 11);
            this.btnLoad.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.btnLoad.Name = "btnLoad";
            this.btnLoad.Size = new System.Drawing.Size(75, 26);
            this.btnLoad.TabIndex = 7;
            this.btnLoad.Text = "Load";
            this.btnLoad.UseVisualStyleBackColor = true;
            this.btnLoad.Click += new System.EventHandler(this.btnLoad_Click);
            // 
            // btnMerge
            // 
            this.btnMerge.Location = new System.Drawing.Point(665, 11);
            this.btnMerge.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.btnMerge.Name = "btnMerge";
            this.btnMerge.Size = new System.Drawing.Size(156, 26);
            this.btnMerge.TabIndex = 12;
            this.btnMerge.Text = "Merge To Scratch";
            this.btnMerge.UseVisualStyleBackColor = true;
            this.btnMerge.Click += new System.EventHandler(this.btnMerge_Click);
            // 
            // rbSettings
            // 
            this.rbSettings.AutoSize = true;
            this.rbSettings.Checked = true;
            this.rbSettings.Location = new System.Drawing.Point(3, 2);
            this.rbSettings.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.rbSettings.Name = "rbSettings";
            this.rbSettings.Size = new System.Drawing.Size(80, 21);
            this.rbSettings.TabIndex = 8;
            this.rbSettings.TabStop = true;
            this.rbSettings.Tag = "0";
            this.rbSettings.Text = "Settings";
            this.rbSettings.UseVisualStyleBackColor = true;
            // 
            // kowhaiTreeSettings
            // 
            this.kowhaiTreeSettings.ContextMenuEnabled = true;
            this.kowhaiTreeSettings.Dock = System.Windows.Forms.DockStyle.Fill;
            this.kowhaiTreeSettings.Location = new System.Drawing.Point(3, 27);
            this.kowhaiTreeSettings.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.kowhaiTreeSettings.Name = "kowhaiTreeSettings";
            this.kowhaiTreeSettings.Size = new System.Drawing.Size(448, 626);
            this.kowhaiTreeSettings.TabIndex = 1;
            // 
            // btnDiff
            // 
            this.btnDiff.Location = new System.Drawing.Point(503, 11);
            this.btnDiff.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.btnDiff.Name = "btnDiff";
            this.btnDiff.Size = new System.Drawing.Size(156, 26);
            this.btnDiff.TabIndex = 13;
            this.btnDiff.Text = "Diff To Scratch";
            this.btnDiff.UseVisualStyleBackColor = true;
            this.btnDiff.Click += new System.EventHandler(this.btnDiff_Click);
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.tableLayoutPanel1.ColumnCount = 3;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 20F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 30F));
            this.tableLayoutPanel1.Controls.Add(this.rbSettings, 0, 0);
            this.tableLayoutPanel1.Controls.Add(this.kowhaiTreeSettings, 0, 1);
            this.tableLayoutPanel1.Controls.Add(this.panel1, 1, 1);
            this.tableLayoutPanel1.Controls.Add(this.kowhaiTreeScratch, 2, 1);
            this.tableLayoutPanel1.Controls.Add(this.label1, 2, 0);
            this.tableLayoutPanel1.Location = new System.Drawing.Point(16, 43);
            this.tableLayoutPanel1.Margin = new System.Windows.Forms.Padding(4);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 2;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 25F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel1.Size = new System.Drawing.Size(908, 655);
            this.tableLayoutPanel1.TabIndex = 14;
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.trackBar2);
            this.panel1.Controls.Add(this.trackBar1);
            this.panel1.Controls.Add(this.pnlScope);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel1.Location = new System.Drawing.Point(454, 25);
            this.panel1.Margin = new System.Windows.Forms.Padding(0);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(181, 630);
            this.panel1.TabIndex = 12;
            // 
            // trackBar2
            // 
            this.trackBar2.Location = new System.Drawing.Point(0, 124);
            this.trackBar2.Name = "trackBar2";
            this.trackBar2.Size = new System.Drawing.Size(177, 56);
            this.trackBar2.TabIndex = 7;
            this.trackBar2.Value = 5;
            this.trackBar2.Scroll += new System.EventHandler(this.trackBar_Scroll);
            // 
            // trackBar1
            // 
            this.trackBar1.Location = new System.Drawing.Point(57, 3);
            this.trackBar1.Name = "trackBar1";
            this.trackBar1.Orientation = System.Windows.Forms.Orientation.Vertical;
            this.trackBar1.Size = new System.Drawing.Size(56, 115);
            this.trackBar1.TabIndex = 6;
            this.trackBar1.Value = 5;
            this.trackBar1.Scroll += new System.EventHandler(this.trackBar_Scroll);
            // 
            // pnlScope
            // 
            this.pnlScope.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.pnlScope.Location = new System.Drawing.Point(4, 529);
            this.pnlScope.Margin = new System.Windows.Forms.Padding(4);
            this.pnlScope.Name = "pnlScope";
            this.pnlScope.Size = new System.Drawing.Size(173, 97);
            this.pnlScope.TabIndex = 5;
            this.pnlScope.Paint += new System.Windows.Forms.PaintEventHandler(this.OnPaint);
            // 
            // kowhaiTreeScratch
            // 
            this.kowhaiTreeScratch.ContextMenuEnabled = true;
            this.kowhaiTreeScratch.Dock = System.Windows.Forms.DockStyle.Fill;
            this.kowhaiTreeScratch.Location = new System.Drawing.Point(638, 27);
            this.kowhaiTreeScratch.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.kowhaiTreeScratch.Name = "kowhaiTreeScratch";
            this.kowhaiTreeScratch.Size = new System.Drawing.Size(267, 626);
            this.kowhaiTreeScratch.TabIndex = 13;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(638, 0);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(56, 17);
            this.label1.TabIndex = 14;
            this.label1.Text = "Scratch";
            // 
            // btnLoadScratch
            // 
            this.btnLoadScratch.Location = new System.Drawing.Point(341, 11);
            this.btnLoadScratch.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.btnLoadScratch.Name = "btnLoadScratch";
            this.btnLoadScratch.Size = new System.Drawing.Size(156, 26);
            this.btnLoadScratch.TabIndex = 15;
            this.btnLoadScratch.Text = "Load To Scratch";
            this.btnLoadScratch.UseVisualStyleBackColor = true;
            this.btnLoadScratch.Click += new System.EventHandler(this.btnLoadScratch_Click);
            // 
            // hid
            // 
            this.hid.ProductId = 1152;
            this.hid.VendorId = 5824;
            this.hid.OnSpecifiedDeviceArrived += new System.EventHandler(this.hid_OnSpecifiedDeviceArrived);
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 16F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(940, 713);
            this.Controls.Add(this.btnLoadScratch);
            this.Controls.Add(this.tableLayoutPanel1);
            this.Controls.Add(this.btnDiff);
            this.Controls.Add(this.btnLoad);
            this.Controls.Add(this.btnSave);
            this.Controls.Add(this.btnMerge);
            this.Controls.Add(this.btnRefreshTrees);
            this.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.Name = "MainForm";
            this.Text = "MainForm";
            this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.MainForm_FormClosed);
            this.Load += new System.EventHandler(this.MainForm_Load);
            this.tableLayoutPanel1.ResumeLayout(false);
            this.tableLayoutPanel1.PerformLayout();
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar2)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar1)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Button btnRefreshTrees;
        private kowhai_sharp.KowhaiTree kowhaiTreeSettings;
        private System.Windows.Forms.Button btnSave;
        private System.Windows.Forms.Button btnLoad;
        private System.Windows.Forms.RadioButton rbSettings;
        private System.Windows.Forms.Button btnMerge;
        private System.Windows.Forms.Button btnDiff;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.Panel pnlScope;
        private kowhai_sharp.KowhaiTree kowhaiTreeScratch;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Button btnLoadScratch;
        private UsbLibrary.UsbHidPort hid;
        private System.Windows.Forms.TrackBar trackBar2;
        private System.Windows.Forms.TrackBar trackBar1;
    }
}
