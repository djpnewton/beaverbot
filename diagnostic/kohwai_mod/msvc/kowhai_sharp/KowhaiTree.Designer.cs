namespace kowhai_sharp
{
    partial class KowhaiTree
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

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.treeView1 = new System.Windows.Forms.TreeView();
            this.contextMenuStrip1 = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.refreshNodeToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.editNodeToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.contextMenuStrip1.SuspendLayout();
            this.SuspendLayout();
            // 
            // treeView1
            // 
            this.treeView1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.treeView1.Location = new System.Drawing.Point(0, 0);
            this.treeView1.Margin = new System.Windows.Forms.Padding(2);
            this.treeView1.Name = "treeView1";
            this.treeView1.Size = new System.Drawing.Size(112, 122);
            this.treeView1.TabIndex = 0;
            this.treeView1.AfterLabelEdit += new System.Windows.Forms.NodeLabelEditEventHandler(this.treeView1_AfterLabelEdit);
            this.treeView1.DoubleClick += new System.EventHandler(this.treeView1_DoubleClick);
            this.treeView1.KeyDown += new System.Windows.Forms.KeyEventHandler(this.treeView1_KeyDown);
            this.treeView1.MouseDown += new System.Windows.Forms.MouseEventHandler(this.treeView1_MouseDown);
            this.treeView1.MouseUp += new System.Windows.Forms.MouseEventHandler(this.treeView1_MouseUp);
            // 
            // contextMenuStrip1
            // 
            this.contextMenuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.refreshNodeToolStripMenuItem,
            this.editNodeToolStripMenuItem});
            this.contextMenuStrip1.Name = "contextMenuStrip1";
            this.contextMenuStrip1.Size = new System.Drawing.Size(153, 70);
            // 
            // refreshNodeToolStripMenuItem
            // 
            this.refreshNodeToolStripMenuItem.Name = "refreshNodeToolStripMenuItem";
            this.refreshNodeToolStripMenuItem.Size = new System.Drawing.Size(152, 22);
            this.refreshNodeToolStripMenuItem.Text = "Refresh Node";
            this.refreshNodeToolStripMenuItem.Click += new System.EventHandler(this.refreshNodeToolStripMenuItem_Click);
            // 
            // editNodeToolStripMenuItem
            // 
            this.editNodeToolStripMenuItem.Name = "editNodeToolStripMenuItem";
            this.editNodeToolStripMenuItem.Size = new System.Drawing.Size(152, 22);
            this.editNodeToolStripMenuItem.Text = "Edit Node";
            this.editNodeToolStripMenuItem.Click += new System.EventHandler(this.editNodeToolStripMenuItem_Click);
            // 
            // KowhaiTree
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.treeView1);
            this.Margin = new System.Windows.Forms.Padding(2);
            this.Name = "KowhaiTree";
            this.Size = new System.Drawing.Size(112, 122);
            this.contextMenuStrip1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TreeView treeView1;
        private System.Windows.Forms.ContextMenuStrip contextMenuStrip1;
        private System.Windows.Forms.ToolStripMenuItem refreshNodeToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem editNodeToolStripMenuItem;
    }
}
