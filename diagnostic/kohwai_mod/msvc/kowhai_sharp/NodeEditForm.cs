using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace kowhai_sharp
{
    public partial class NodeEditForm : Form
    {
        public NodeEditForm()
        {
            InitializeComponent();
            kowhaiTree1.ContextMenuEnabled = false;
            kowhaiTree1.DataChange += new KowhaiTree.DataChangeEventHandler(kowhaiTree1_DataChange);
            kowhaiTree1.NodeRead += new KowhaiTree.NodeReadEventHandler(kowhaiTree1_NodeRead);
        }

        void kowhaiTree1_DataChange(object sender, KowhaiTree.DataChangeEventArgs e)
        {
            kowhaiTree1.UpdateData(e.Buffer, e.Info.Offset);
            tbHex.Text = BytesToHexString(kowhaiTree1.GetData());
        }

        void kowhaiTree1_NodeRead(object sender, KowhaiTree.NodeReadEventArgs e)
        {
        }

        public void UpdateTree(Kowhai.kowhai_node_t[] descriptor, string[] symbols, byte[] data, KowhaiTree.KowhaiNodeInfo info)
        {
            kowhaiTree1.UpdateDescriptor(descriptor, symbols, info);
            kowhaiTree1.UpdateData(data, 0);
            tbHex.Text = BytesToHexString(data);
        }

        private string BytesToHexString(byte[] data)
        {
            string hex = BitConverter.ToString(data);
            hex = hex.Replace("-", string.Empty);
            return hex;
        }

        private byte[] HexStringToBytes(string hex)
        {
            byte[] bytes = new byte[hex.Length / 2];
            for (int i = 0; i < bytes.Length; i++)
                bytes[i] = Convert.ToByte(hex.Substring(i * 2, 2), 16);
            return bytes;
        }

        public byte[] GetData()
        {
            return kowhaiTree1.GetData();
        }

        private void tbHex_Validated(object sender, EventArgs e)
        {
            try
            {
                kowhaiTree1.UpdateData(HexStringToBytes(tbHex.Text), 0);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
    }
}
