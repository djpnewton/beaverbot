using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace kowhai_sharp
{
    public partial class KowhaiTree : UserControl
    {
        public class KowhaiNodeInfo
        {
            public Kowhai.kowhai_node_t KowhaiNode;
            public int NodeIndex;
            public bool IsArrayItem;
            public ushort ArrayIndex;
            public ushort Offset;
            public KowhaiNodeInfo Parent;
            public KowhaiNodeInfo(Kowhai.kowhai_node_t kowhaiNode, int nodeIndex, bool isArrayItem, ushort arrayIndex, ushort offset, KowhaiNodeInfo parent)
            {
                KowhaiNode = kowhaiNode;
                NodeIndex = nodeIndex;
                IsArrayItem = isArrayItem;
                ArrayIndex = arrayIndex;
                Offset = offset;
                Parent = parent;
            }
        }

        public class DataChangeEventArgs : EventArgs
        {
            public KowhaiNodeInfo Info;
            public byte[] Buffer;
            public DataChangeEventArgs(KowhaiNodeInfo info, byte[] buffer)
            {
                Info = info;
                Buffer = buffer;
            }
        }

        public delegate void DataChangeEventHandler(object sender, DataChangeEventArgs e);

        public class NodeReadEventArgs : EventArgs
        {
            public KowhaiNodeInfo Info;
            public NodeReadEventArgs(KowhaiNodeInfo info)
            {
                Info = info;
            }
        }

        public bool ContextMenuEnabled { get; set; }

        public delegate void NodeReadEventHandler(object sender, NodeReadEventArgs e);

        Kowhai.kowhai_node_t[] descriptor;
        string[] symbols;
        byte[] data;

        public event DataChangeEventHandler DataChange;
        public event NodeReadEventHandler NodeRead;

        public KowhaiTree()
        {
            InitializeComponent();
            ContextMenuEnabled = true;
        }

        string GetDataTypeString(int dataType)
        {
            dataType = Kowhai.RawDataType(dataType);
            switch (dataType)
            {
                case Kowhai.INT8:
                    return "int8";
                case Kowhai.UINT8:
                    return "uint8";
                case Kowhai.CHAR:
                    return "char";
                case Kowhai.INT16:
                    return "int16";
                case Kowhai.UINT16:
                    return "uint16";
                case Kowhai.INT32:
                    return "int32";
                case Kowhai.UINT32:
                    return "uint32";
                case Kowhai.FLOAT:
                    return "float";
            }
            return "error";
        }

        byte[] GetNodeData(KowhaiNodeInfo info)
        {
            if (info == null)
                return null;
            int size = Kowhai.kowhai_get_node_type_size(info.KowhaiNode.type);
            if (info.Offset + size <= data.Length)
            {
                byte[] nodeData = new byte[size];
                Array.Copy(data, info.Offset, nodeData, 0, size);
                return nodeData;
            }
            return null;
        }

        private object GetDataValue(KowhaiNodeInfo info)
        {
            if (info == null)
                return null;
            int size = Kowhai.kowhai_get_node_type_size(info.KowhaiNode.type);
            if (info.Offset + size <= data.Length)
            {
                switch (info.KowhaiNode.type)
                {
                    case Kowhai.INT8:
                        return (sbyte)data[info.Offset];
                    case Kowhai.UINT8:
                        return data[info.Offset];
                    case Kowhai.CHAR:
                        // convert byte array to string
                        string result = "";
                        int max = Math.Min(data.Length, info.Offset + info.KowhaiNode.count) - info.Offset;
                        result = System.Text.Encoding.ASCII.GetString(data, info.Offset, max);
                        int nullLocation = result.IndexOf((char)0);
                        if (nullLocation > -1)
                            return result.Substring(0, nullLocation);
                        return result;
                    case Kowhai.INT16:
                        return BitConverter.ToInt16(data, info.Offset);
                    case Kowhai.UINT16:
                        return BitConverter.ToUInt16(data, info.Offset);
                    case Kowhai.INT32:
                        return BitConverter.ToInt32(data, info.Offset);
                    case Kowhai.UINT32:
                        return BitConverter.ToUInt32(data, info.Offset);
                    case Kowhai.FLOAT:
                        return BitConverter.ToSingle(data, info.Offset);
                }
            }
            return null;
        }

        private byte[] TextToData(string text, ushort dataType)
        {
            byte[] result = new byte[text.Length];
            switch (dataType)
            {
                case Kowhai.INT8:
                    return new byte[] { (byte)Convert.ToSByte(text) };
                case Kowhai.UINT8:
                    return new byte[] { Convert.ToByte(text) };
                case Kowhai.INT16:
                    return BitConverter.GetBytes(Convert.ToInt16(text));
                case Kowhai.UINT16:
                    return BitConverter.GetBytes(Convert.ToUInt16(text));
                case Kowhai.INT32:
                    return BitConverter.GetBytes(Convert.ToInt32(text));
                case Kowhai.UINT32:
                    return BitConverter.GetBytes(Convert.ToUInt32(text));
                case Kowhai.FLOAT:
                    return BitConverter.GetBytes(Convert.ToSingle(text));
                case Kowhai.CHAR:
                    // convert string to a byte array
                    ASCIIEncoding enc = new ASCIIEncoding();
                    result = enc.GetBytes(text + char.MinValue);
                    return result;
            }
            return null;
        }

        string GetNodeTagString(Kowhai.kowhai_node_t node)
        {
            if (node.tag > 0)
                return string.Format("({0})", node.tag);
            return "";
        }

        string GetNodeArrayString(Kowhai.kowhai_node_t node)
        {
            if (node.count > 1)
                return string.Format("[{0}]", node.count);
            return "";
        }

        string GetNodeName(Kowhai.kowhai_node_t node, KowhaiNodeInfo info)
        {
            if (node.type == Kowhai.BRANCH)
                return string.Format("{0}{1}{2}", symbols[node.symbol], GetNodeArrayString(node), GetNodeTagString(node));
            if (info != null && info.IsArrayItem)
                return string.Format("#{0}{1} = {2}", info.ArrayIndex, GetNodeTagString(node), GetDataValue(info));
            else if (node.type == Kowhai.CHAR)
                return string.Format("{0}{1}{2}: {3} = \"{4}\"", symbols[node.symbol], GetNodeArrayString(node), GetNodeTagString(node), GetDataTypeString(node.type), GetDataValue(info));
            else if (node.count > 1)
                return string.Format("{0}{1}{2}: {3}", symbols[node.symbol], GetNodeArrayString(node), GetNodeTagString(node), GetDataTypeString(node.type));
            else
                return string.Format("{0}{1}: {2} = {3}", symbols[node.symbol], GetNodeTagString(node), GetDataTypeString(node.type), GetDataValue(info));
        }

        void _UpdateDescriptor(Kowhai.kowhai_node_t[] descriptor, ref int index, ref ushort offset, TreeNode node, KowhaiNodeInfo initialNodeInfo)
        {
            while (index < descriptor.Length)
            {
                Kowhai.kowhai_node_t descNode = descriptor[index];
                switch (descNode.type)
                {
                    case Kowhai.BRANCH:
                        if (node == null)
                            node = treeView1.Nodes.Add(GetNodeName(descNode, null));
                        else
                            node = node.Nodes.Add(GetNodeName(descNode, null));
                        KowhaiNodeInfo parentInfo = null;
                        if (node.Parent != null)
                            parentInfo = (KowhaiNodeInfo)node.Parent.Tag;
                        node.Tag = new KowhaiNodeInfo(descNode, index, false, 0, offset, parentInfo);
                        if (descNode.count > 1)
                        {
                            int prevIndex = index;
                            for (ushort i = 0; i < descNode.count; i++)
                            {
                                if (initialNodeInfo != null && initialNodeInfo.IsArrayItem && initialNodeInfo.ArrayIndex != i)
                                    continue;
                                index = prevIndex;
                                TreeNode arrayNode = node.Nodes.Add("#" + i.ToString());
                                arrayNode.Tag = new KowhaiNodeInfo(descNode, index, true, i, offset, parentInfo);
                                index++;
                                _UpdateDescriptor(descriptor, ref index, ref offset, arrayNode, null);
                            }
                        }
                        else
                        {
                            index++;
                            _UpdateDescriptor(descriptor, ref index, ref offset, node, null);
                        }
                        node = node.Parent;
                        break;
                    case Kowhai.BRANCH_END:
                        return;
                    default:
                        TreeNode leaf;
                        if (node == null)
                            leaf = treeView1.Nodes.Add(GetNodeName(descNode, null));
                        else
                            leaf = node.Nodes.Add(GetNodeName(descNode, null));
                        KowhaiNodeInfo parentNodeInfo = null;
                        if (leaf.Parent != null)
                            parentNodeInfo = (KowhaiNodeInfo)leaf.Parent.Tag;
                        leaf.Tag = new KowhaiNodeInfo(descNode, index, false, 0, offset, parentNodeInfo);
                        if (descNode.count > 1 && descNode.type != Kowhai.CHAR)
                        {
                            for (ushort i = 0; i < descNode.count; i++)
                            {
                                TreeNode child = leaf.Nodes.Add("#" + i.ToString());
                                child.Tag = new KowhaiNodeInfo(descNode, index, true, i, offset, parentNodeInfo);
                                offset += (ushort)Kowhai.kowhai_get_node_type_size(descNode.type);
                            }
                        }
                        else
                            offset += (ushort)(Kowhai.kowhai_get_node_type_size(descNode.type) * descNode.count);
                        break;
                }
                index++;
            }
        }

        public void UpdateDescriptor(Kowhai.kowhai_node_t[] descriptor, string[] symbols, KowhaiNodeInfo info)
        {
            this.descriptor = descriptor;
            this.symbols = symbols;
            treeView1.Nodes.Clear();
            int index = 0;
            ushort offset = 0;
            _UpdateDescriptor(descriptor, ref index, ref offset, null, info);
            treeView1.ExpandAll();
            return;
        }

        void UpdateTreeNodeData(TreeNodeCollection nodes)
        {
            foreach (TreeNode node in nodes)
            {
                if (node.Nodes.Count > 0)
                    UpdateTreeNodeData(node.Nodes);
                else if (node.Tag != null)
                {
                    KowhaiNodeInfo info = (KowhaiNodeInfo)node.Tag;
                    string newName = GetNodeName(info.KowhaiNode, info);
                    if (newName != node.Text)
                        node.Text = newName;
                }
            }
        }

        public void UpdateData(byte[] newData, int offset)
        {
            int maxSize = offset + newData.Length;
            if (data == null)
                data = new byte[maxSize];
            if (data.Length < maxSize)
                Array.Resize<byte>(ref data, maxSize);
            Array.Copy(newData, 0, data, offset, newData.Length);

            treeView1.BeginUpdate();
            UpdateTreeNodeData(treeView1.Nodes);
            treeView1.EndUpdate();
        }

        public new void Update()
        {
            UpdateDescriptor(descriptor, symbols, null);
            UpdateData(data, 0);
        }

        public Kowhai.kowhai_node_t[] GetDescriptor()
        {
            return descriptor;
        }

        public byte[] GetData()
        {
            return data;
        }

        TreeNode selectedNode;

        private void treeView1_MouseDown(object sender, MouseEventArgs e)
        {
            selectedNode = treeView1.GetNodeAt(e.X, e.Y);
        }

        private void treeView1_MouseUp(object sender, MouseEventArgs e)
        {
            if (ContextMenuEnabled && e.Button == System.Windows.Forms.MouseButtons.Right && selectedNode != null)
            {
                treeView1.SelectedNode = selectedNode;
                contextMenuStrip1.Show(this, new Point(e.X, e.Y));
            }
        }

        private void treeView1_DoubleClick(object sender, EventArgs e)
        {
            if (selectedNode != null)
            {
                treeView1.SelectedNode = selectedNode;
                BeginEdit(selectedNode);
            }
        }

        private void BeginEdit(TreeNode node)
        {
            if (data != null && node != null && !node.IsEditing)
            {
                if (node.Tag != null)
                {
                    if (node.Nodes.Count == 0)
                    {
                        KowhaiNodeInfo info = (KowhaiNodeInfo)node.Tag;
                        object dataValue = GetDataValue(info);
                        if (dataValue != null)
                        {
                            treeView1.LabelEdit = true;
                            node.Text = dataValue.ToString();
                            node.BeginEdit();
                        }
                    }
                }
            }
        }

        private void treeView1_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Return || e.KeyCode == Keys.Enter)
                BeginEdit(treeView1.SelectedNode);
        }

        private void treeView1_AfterLabelEdit(object sender, NodeLabelEditEventArgs e)
        {
            e.CancelEdit = true;
            KowhaiNodeInfo info = (KowhaiNodeInfo)e.Node.Tag;
            if (e.Label != null)
            {
                byte[] data;
                try
                {
                    data = TextToData(e.Label, info.KowhaiNode.type);
                    e.Node.Text = "updating...";
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }
                if (DataChange != null)
                    DataChange(this, new DataChangeEventArgs(info, data));
            }
            else
                e.Node.Text = GetNodeName(info.KowhaiNode, info);
            treeView1.LabelEdit = false;
        }

        private void refreshNodeToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (selectedNode != null && selectedNode.Tag != null)
            {
                BlankNodes(selectedNode);
                KowhaiNodeInfo info = (KowhaiNodeInfo)selectedNode.Tag;
                if (NodeRead != null)
                    NodeRead(this, new NodeReadEventArgs(info));
            }
        }

        private void editNodeToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (selectedNode != null && selectedNode.Tag != null)
            {
                KowhaiNodeInfo info = (KowhaiNodeInfo)selectedNode.Tag;
                bool branch = info.KowhaiNode.type == Kowhai.BRANCH;
                bool leafArrayParent = !branch && info.KowhaiNode.count > 1 && info.IsArrayItem == false;
                if (branch | leafArrayParent)
                {
                    // create sub branch descriptor and data
                    List<Kowhai.kowhai_node_t> descBranch = new List<Kowhai.kowhai_node_t>();
                    if (leafArrayParent)
                        descBranch.Add(info.KowhaiNode);
                    else
                    {
                        int depth = 0;
                        for (int i = info.NodeIndex; i < descriptor.Length; i++)
                        {
                            Kowhai.kowhai_node_t node = descriptor[i];
                            descBranch.Add(node);
                            if (node.type == Kowhai.BRANCH)
                                depth++;
                            else if (node.type == Kowhai.BRANCH_END)
                                depth--;
                            if (depth == 0)
                                break;
                        }
                    }
                    int size;
                    Kowhai.kowhai_node_t[] descBranchArray = descBranch.ToArray();
                    Kowhai.GetNodeSize(descBranchArray, out size);
                    if (info.IsArrayItem)
                        size /= info.KowhaiNode.count;
                    byte[] dataBranchArray = new byte[size];
                    Array.Copy(data, info.Offset, dataBranchArray, 0, size);

                    // show form with sub branch
                    NodeEditForm f = new NodeEditForm();
                    f.UpdateTree(descBranchArray, symbols, dataBranchArray, info);
                    f.Location = PointToScreen(selectedNode.Bounds.Location);
                    if (f.ShowDialog() == DialogResult.OK)
                    {
                        BlankNodes(selectedNode);
                        DataChange(this, new DataChangeEventArgs(info, f.GetData()));
                    }
                }
                else
                    BeginEdit(selectedNode);
            }
        }

        private void BlankNodes(TreeNode node)
        {
            treeView1.BeginUpdate();
            _blankNodes(node);
            treeView1.EndUpdate();
        }

        const string blank = "...";
        private void _blankNodes(TreeNode node)
        {
            if (node.Nodes.Count == 0)
                node.Text = blank;
            foreach (TreeNode childNode in node.Nodes)
                BlankNodes(childNode);
        }

        public void DiffAt(Kowhai.kowhai_symbol_t[] symbolPath)
        {
            treeView1.BeginUpdate();
            foreach (TreeNode childNode in treeView1.Nodes)
                _diffAt(symbolPath, new bool[symbolPath.Length], 0, childNode);
            treeView1.EndUpdate();
        }

        public void _diffAt(Kowhai.kowhai_symbol_t[] symbolPath, bool[] symbolPathMatchesArrayIndex, int symbolPathIndex, TreeNode node)
        {
            KowhaiNodeInfo info = (KowhaiNodeInfo)node.Tag;
            bool isBranchArrayItem = info.IsArrayItem && info.KowhaiNode.type == Kowhai.BRANCH;
            bool isLeafArrayParent = node.Nodes.Count > 0 && info.KowhaiNode.type != Kowhai.BRANCH;
            // poplulate symbolPathMatchesArrayIndex to ensure we are on the correct branch of the treeview
            if (isBranchArrayItem)
                symbolPathMatchesArrayIndex[symbolPathIndex - 1] = info.ArrayIndex == symbolPath[symbolPathIndex - 1].parts.array_index;
            else
            {
                if (info.KowhaiNode.symbol != symbolPath[symbolPathIndex].parts.name)
                    return;
                symbolPathMatchesArrayIndex[symbolPathIndex] = info.ArrayIndex == symbolPath[symbolPathIndex].parts.array_index;
            }
            // color treeview node red is match is found
            if (symbolPathIndex == symbolPath.Length - 1 && !symbolPathMatchesArrayIndex.Contains(false) &&
                !isLeafArrayParent && !isBranchArrayItem)
                node.BackColor = Color.Red;
            // increment symbol path index and check next level of treeview
            if (!isBranchArrayItem && !isLeafArrayParent)
                symbolPathIndex++;
            foreach (TreeNode childNode in node.Nodes)
                _diffAt(symbolPath, symbolPathMatchesArrayIndex, symbolPathIndex, childNode);
        }

        public void ResetNodesBackColor()
        {
            treeView1.BeginUpdate();
            foreach (TreeNode childNode in treeView1.Nodes)
                _resetNodesBackColor(childNode);
            treeView1.EndUpdate();
        }

        private void _resetNodesBackColor(TreeNode node)
        {
            if (node.Nodes.Count == 0)
                node.BackColor = Color.White;
            foreach (TreeNode childNode in node.Nodes)
                _resetNodesBackColor(childNode);
        }
    }
}
