using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Collections;
using kowhai_sharp;

namespace kowhai_test
{
    public partial class MainForm : Form
    {
        const int TREE_ID_TEENSY = 0;

        //Sock sock;
        const int PACKET_SIZE = 64;
        List<Kowhai.kowhai_node_t[]> descriptors = new List<Kowhai.kowhai_node_t[]>();

        public MainForm()
        {
            InitializeComponent();
        }

        private void MainForm_Load(object sender, EventArgs e)
        {
            //sock = new Sock();
            //if (sock.Connect())
            hid.OnDataRecieved += new UsbLibrary.DataRecievedEventHandler(hid_OnDataRecieved);
            hid.CheckDevicePresent();
            if (hid.SpecifiedDevice != null)
            {
                btnRefreshTrees.Enabled = true;
                //sock.SockBufferReceived += new SockReceiveEventHandler(sock_SockBufferReceived);
                //sock.StartAsyncReceives(new byte[PACKET_SIZE], PACKET_SIZE);
                kowhaiTreeSettings.DataChange += new KowhaiTree.DataChangeEventHandler(kowhaiTree_DataChange);
            }
            else
                btnRefreshTrees.Enabled = false;
        }

        private void MainForm_FormClosed(object sender, FormClosedEventArgs e)
        {
            //if (btnRefreshTrees.Enabled)
              //  sock.Disconnect();
        }

        void hid_OnDataRecieved(object sender, UsbLibrary.DataRecievedEventArgs args)
        {
            byte[] buffer = new byte[args.data.Length - 1];
            Array.Copy(args.data, 1, buffer, 0, args.data.Length - 1);
            this.BeginInvoke((MethodInvoker)delegate
            {
                ProcessPacket(buffer);
            });
        }

        void sock_SockBufferReceived(object sender, SockReceiveEventArgs e)
        {
            byte[] buffer = new byte[e.Size];
            Array.Copy(e.Buffer, buffer, e.Size);
            this.BeginInvoke((MethodInvoker)delegate
            {
                ProcessPacket(buffer);
            });
        }

        byte[] ScopePointData = null;
        List<UInt16> ScopePoints = new List<UInt16>();
        UInt16 ScopeMinVal = UInt16.MaxValue, ScopeMaxVal = UInt16.MinValue;
        private void ProcessPacket(byte[] buffer)
        {
            KowhaiProtocol.kowhai_protocol_t prot;
            Kowhai.kowhai_symbol_t[] symbols;
            if (KowhaiProtocol.Parse(buffer, buffer.Length, out prot, out symbols) == Kowhai.STATUS_OK)
            {
                while (prot.header.tree_id > descriptors.Count - 1)
                    descriptors.Add(null);
                Kowhai.kowhai_node_t[] descriptor = descriptors[prot.header.tree_id];

                switch (prot.header.command)
                {
                    case KowhaiProtocol.CMD_READ_DATA_ACK:
                    case KowhaiProtocol.CMD_WRITE_DATA_ACK:
                    case KowhaiProtocol.CMD_READ_DATA_ACK_END:
                        byte[] data = KowhaiProtocol.GetBuffer(prot);
                        int nodeOffset;
                        Kowhai.kowhai_node_t node;
                        if (Kowhai.GetNode(descriptor, symbols, out nodeOffset, out node) == Kowhai.STATUS_OK)
                        {
                            KowhaiTree tree = GetKowhaiTree(prot.header.tree_id);
                            tree.UpdateData(data, nodeOffset + prot.payload.spec.data.memory.offset);
                            /*
                            if (tree == kowhaiTreeScope)
                            {
                                Kowhai.kowhai_symbol_t[] symbolPath = new Kowhai.kowhai_symbol_t[] {
                                    new Kowhai.kowhai_symbol_t((uint)KowhaiSymbols.Symbols.Constants.Scope),
                                    new Kowhai.kowhai_symbol_t((uint)KowhaiSymbols.Symbols.Constants.Pixels)
                                };
                                if (Kowhai.GetNode(tree.GetDescriptor(), symbolPath, out nodeOffset, out node) == Kowhai.STATUS_OK)
                                {
                                    // update the scope points the the new data
                                    if (ScopePointData == null || ScopePointData.Length != node.count * 2)
                                        Array.Resize(ref ScopePointData, node.count * 2);
                                    int arrayIndex = 0;
                                    if (symbols.Length == 2)
                                        arrayIndex = symbols[1].parts.array_index;
                                    Array.Copy(data, 0, ScopePointData, arrayIndex * 2 + prot.payload.spec.data.memory.offset, data.Length);
                                    for (int i = 0; i < ScopePointData.Length / 2; i++)
                                    {
                                        UInt16 value = BitConverter.ToUInt16(ScopePointData, i * 2);
                                        if (value > ScopeMaxVal)
                                            ScopeMaxVal = value;
                                        if (value < ScopeMinVal)
                                            ScopeMinVal = value;
                                        if (ScopePoints.Count > i)
                                            ScopePoints[i] = value;
                                        else
                                            ScopePoints.Add(value);
                                    }
                                    pnlScope.Invalidate();
                                }
                            }*/
                        }
                        break;
                    case KowhaiProtocol.CMD_READ_DESCRIPTOR_ACK:
                    case KowhaiProtocol.CMD_READ_DESCRIPTOR_ACK_END:
                        if (descriptor == null || descriptor.Length < prot.payload.spec.descriptor.node_count)
                        {
                            Array.Resize<Kowhai.kowhai_node_t>(ref descriptor, prot.payload.spec.descriptor.node_count);
                            descriptors[prot.header.tree_id] = descriptor;
                        }
                        KowhaiProtocol.CopyDescriptor(descriptor, prot.payload);

                        if (prot.header.command == KowhaiProtocol.CMD_READ_DESCRIPTOR_ACK_END)
                        {
                            GetKowhaiTree(prot.header.tree_id).UpdateDescriptor(descriptor, KowhaiSymbols.Symbols.Strings, null);

                            buffer = new byte[PACKET_SIZE];
                            int bytesRequired;
                            prot.header.command = KowhaiProtocol.CMD_READ_DATA;
                            if (KowhaiProtocol.Create(buffer, PACKET_SIZE, ref prot,
                                GetRootSymbolPath(prot.header.tree_id),
                                out bytesRequired) == Kowhai.STATUS_OK)
                                //sock.Send(buffer, bytesRequired);
                                SendData(buffer, bytesRequired);
                        }
                        break;
                }
            }
        }

        private List<ushort> CreateNodeInfoArrayIndexList(KowhaiTree.KowhaiNodeInfo info)
        {
            List<ushort> arrayIndexes = new List<ushort>();
            while (info != null)
            {
                arrayIndexes.Add(info.ArrayIndex);
                info = info.Parent;
            }
            arrayIndexes.Reverse();
            return arrayIndexes;
        }

        void kowhaiTree_DataChange(object sender, KowhaiTree.DataChangeEventArgs e)
        {
            Application.DoEvents();
            System.Threading.Thread.Sleep(250);

            byte[] buffer = new byte[PACKET_SIZE];
            List<ushort> arrayIndexes = CreateNodeInfoArrayIndexList(e.Info);
            Kowhai.kowhai_symbol_t[] symbols = Kowhai.GetSymbolPath(GetDescriptor(sender), e.Info.KowhaiNode, e.Info.NodeIndex, arrayIndexes.ToArray());
            KowhaiProtocol.kowhai_protocol_t prot = new KowhaiProtocol.kowhai_protocol_t();
            prot.header.tree_id = GetTreeId(sender);
            prot.header.command = KowhaiProtocol.CMD_WRITE_DATA;
            int bytesRequired;
            KowhaiProtocol.Create(buffer, PACKET_SIZE, ref prot, symbols, out bytesRequired);
            int overhead;
            KowhaiProtocol.kowhai_protocol_get_overhead(ref prot, out overhead);
            int offset = 0;
            int maxPayloadSize = PACKET_SIZE - overhead;
            while (e.Buffer.Length - offset > maxPayloadSize)
            {
                KowhaiProtocol.Create(buffer, PACKET_SIZE, ref prot, symbols, CopyArray(e.Buffer, offset, maxPayloadSize), (ushort)offset, out bytesRequired);
                //sock.Send(buffer, bytesRequired);
                SendData(buffer, bytesRequired);
                offset += maxPayloadSize;
            }
            KowhaiProtocol.Create(buffer, PACKET_SIZE, ref prot, symbols, CopyArray(e.Buffer, offset, e.Buffer.Length - offset), (ushort)offset, out bytesRequired);
            //sock.Send(buffer, bytesRequired);
            SendData(buffer, bytesRequired);
        }

        private byte[] CopyArray(byte[] source, int offset, int size)
        {
            byte[] data = new byte[size];
            Array.Copy(source, offset, data, 0, size);
            return data;
        }

        void kowhaiTree_NodeRead(object sender, KowhaiTree.NodeReadEventArgs e)
        {
            Application.DoEvents();
            System.Threading.Thread.Sleep(250);

            byte[] buffer = new byte[PACKET_SIZE];
            List<ushort> arrayIndexes = CreateNodeInfoArrayIndexList(e.Info);
            Kowhai.kowhai_symbol_t[] symbols = Kowhai.GetSymbolPath(GetDescriptor(sender), e.Info.KowhaiNode, e.Info.NodeIndex, arrayIndexes.ToArray());
            KowhaiProtocol.kowhai_protocol_t prot = new KowhaiProtocol.kowhai_protocol_t();
            prot.header.tree_id = GetTreeId(sender);
            prot.header.command = KowhaiProtocol.CMD_READ_DATA;
            int bytesRequired;
            KowhaiProtocol.Create(buffer, PACKET_SIZE, ref prot, symbols, out bytesRequired);
            //sock.Send(buffer, bytesRequired);
            SendData(buffer, bytesRequired);
        }

        private void btnRefreshTrees_Click(object sender, EventArgs e)
        {
            byte[] buffer = new byte[2];
            buffer[0] = TREE_ID_TEENSY;
            buffer[1] = KowhaiProtocol.CMD_READ_DESCRIPTOR;
            //sock.Send(buffer, 2);
            SendData(buffer, 2);
        }

        string getSymbolName(Object param, UInt16 value)
        {
            return KowhaiSymbols.Symbols.Strings[value];
        }

        private void btnSave_Click(object sender, EventArgs e)
        {
            KowhaiTree tree = GetTreeFromRadioButtonSelection();
            string text;
            if (KowhaiSerialize.Serialize(tree.GetDescriptor(), tree.GetData(), out text, 0x1000, null, getSymbolName) == Kowhai.STATUS_OK)
            {
                SaveFileDialog d = new SaveFileDialog();
                d.Filter = "Kowhai Files | *.kowhai";
                d.DefaultExt = "kowhai";
                if (d.ShowDialog() == System.Windows.Forms.DialogResult.OK)
                {
                    System.IO.StreamWriter sw = System.IO.File.CreateText(d.FileName);
                    sw.Write(text);
                    sw.Close();
                }
            }
        }

        private void btnLoad_Click(object sender, EventArgs e)
        {
            KowhaiTree tree = GetTreeFromRadioButtonSelection();
            LoadTree(tree);
        }

        private void LoadTree(KowhaiTree tree)
        {
            OpenFileDialog d = new OpenFileDialog();
            d.Filter = "Kowhai Files | *.kowhai";
            d.DefaultExt = "kowhai";
            if (d.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                string text = System.IO.File.ReadAllText(d.FileName);
                Kowhai.kowhai_node_t[] descriptor;
                byte[] data;
                if (KowhaiSerialize.Deserialize(text, out descriptor, out data) == Kowhai.STATUS_OK)
                {
                    tree.UpdateDescriptor(descriptor, KowhaiSymbols.Symbols.Strings, null);
                    tree.UpdateData(data, 0);
                }
            }
        }

        private void btnLoadScratch_Click(object sender, EventArgs e)
        {
            LoadTree(kowhaiTreeScratch);
        }

        void onSettingsDiffRight(Object param, Kowhai.Tree tree, Kowhai.kowhai_symbol_t[] symbolPath)
        {
            kowhaiTreeScratch.DiffAt(symbolPath);
        }

        private void btnDiff_Click(object sender, EventArgs e)
        {
            KowhaiTree leftTree = GetTreeFromRadioButtonSelection();
            Kowhai.Tree left = new Kowhai.Tree(leftTree.GetDescriptor(), leftTree.GetData());
            kowhaiTreeScratch.ResetNodesBackColor();
            Kowhai.Tree right = new Kowhai.Tree(kowhaiTreeScratch.GetDescriptor(), kowhaiTreeScratch.GetData());
            if (KowhaiUtils.Diff(left, right, null, null, onSettingsDiffRight) != Kowhai.STATUS_OK)
                MessageBox.Show("Diff Error", "Doh!", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }

        private void btnMerge_Click(object sender, EventArgs e)
        {
            KowhaiTree srcTree = GetTreeFromRadioButtonSelection();
            Kowhai.Tree src = new Kowhai.Tree(srcTree.GetDescriptor(), srcTree.GetData());
            Kowhai.Tree dst = new Kowhai.Tree(kowhaiTreeScratch.GetDescriptor(), kowhaiTreeScratch.GetData());
            if (KowhaiUtils.Merge(dst, src) == Kowhai.STATUS_OK)
                kowhaiTreeScratch.Update();
            else
                MessageBox.Show("Merge Error", "Doh!", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }

        private Kowhai.kowhai_symbol_t[] GetRootSymbolPath(byte treeId)
        {
            if (treeId == TREE_ID_TEENSY)
                return new Kowhai.kowhai_symbol_t[] { new Kowhai.kowhai_symbol_t((uint)KowhaiSymbols.Symbols.Constants.Teensy) };
            return null;
        }

        private KowhaiTree GetKowhaiTree(byte treeId)
        {
            if (treeId == TREE_ID_TEENSY)
                return kowhaiTreeSettings;
            return null;
        }

        private byte GetTreeId(object sender)
        {
            if (sender == kowhaiTreeSettings)
                return TREE_ID_TEENSY;
            return 255;
        }

        KowhaiTree GetTreeFromRadioButtonSelection()
        {
            if (rbSettings.Checked)
                return kowhaiTreeSettings;
            return null;
        }

        private Kowhai.kowhai_node_t[] GetDescriptor(object sender)
        {
            return descriptors[GetTreeId(sender)];
        }

        private void OnPaint(object sender, PaintEventArgs e)
        {
            // repaint the scope points
            Graphics g = e.Graphics;
            g.SmoothingMode = System.Drawing.Drawing2D.SmoothingMode.AntiAlias;
            int w = pnlScope.Width;
            int h = pnlScope.Height;
            const int border = 10;
            float x = border, dx = (float)(w - border * 2) / (ScopePoints.Count - 1);
            PointF last = new PointF(x, h - border), next;
            PointF[] points = new PointF[ScopePoints.Count];
            g.Clear(Color.White);
            if (ScopePoints.Count > 1)
            {
                for (int i = 0; i < ScopePoints.Count; i++, x += dx)
                {
                    // build the next point
                    UInt16 y = ScopePoints[i];
                    float yf = (h - border * 2) * (y - ScopeMinVal) / (ScopeMaxVal - ScopeMinVal);
                    next = new PointF(x, h - border - yf);
                    points[i] = last;
                    last = next;
                }
                g.DrawLines(Pens.CornflowerBlue, points);
            }
        }

        void SendData(byte[] buffer, int size)
        {
            //sock.Send(buffer, bytesRequired);
            byte[] data = new byte[hid.SpecifiedDevice.OutputReportLength];
            Array.Copy(buffer, 0, data, 1, size);
            hid.SpecifiedDevice.SendData(data);
        }

        private void hid_OnSpecifiedDeviceArrived(object sender, EventArgs e)
        {

        }

        private void trackBar_Scroll(object sender, EventArgs e)
        {
            byte[] buffer = new byte[PACKET_SIZE];
            int bytesRequired;
            KowhaiProtocol.kowhai_protocol_t prot = new KowhaiProtocol.kowhai_protocol_t();
            prot.header.tree_id = TREE_ID_TEENSY;
            prot.header.command = KowhaiProtocol.CMD_WRITE_DATA;

            Kowhai.kowhai_symbol_t[] symbols;
            byte[] data;

            symbols = new Kowhai.kowhai_symbol_t[] { new Kowhai.kowhai_symbol_t((UInt32)KowhaiSymbols.Symbols.Constants.Teensy),
                                                     new Kowhai.kowhai_symbol_t((UInt16)KowhaiSymbols.Symbols.Constants.GPIO, 5),
                                                     new Kowhai.kowhai_symbol_t((UInt32)KowhaiSymbols.Symbols.Constants.PORT) };
            data = new byte[] { 0 };
            if (trackBar1.Value > trackBar1.Maximum / 2)
                data[0] = 9;
            else if (trackBar1.Value < trackBar1.Maximum / 2)
                data[0] = 18;
            KowhaiProtocol.Create(buffer, PACKET_SIZE, ref prot, symbols, data, 0, out bytesRequired);
            SendData(buffer, bytesRequired);

            symbols = new Kowhai.kowhai_symbol_t[] { new Kowhai.kowhai_symbol_t((UInt32)KowhaiSymbols.Symbols.Constants.Teensy),
                                                     new Kowhai.kowhai_symbol_t((UInt32)KowhaiSymbols.Symbols.Constants.TIMER2),
                                                     new Kowhai.kowhai_symbol_t((UInt32)KowhaiSymbols.Symbols.Constants.OCR2A) };
            data = new byte[] { 0 };
            if (trackBar1.Value > trackBar1.Maximum / 2)
                data[0] = (byte)((trackBar1.Value - trackBar1.Maximum / 2) * 10);
            else if (trackBar1.Value < trackBar1.Maximum / 2)
                data[0] = (byte)((Math.Abs(trackBar1.Value - trackBar1.Maximum / 2)) * 10);
            if (trackBar2.Value > trackBar2.Maximum / 2)
                data[0] = (byte)Math.Max(0, data[0] - ((trackBar2.Value - trackBar2.Maximum / 2) * 10));
            KowhaiProtocol.Create(buffer, PACKET_SIZE, ref prot, symbols, data, 0, out bytesRequired);
            SendData(buffer, bytesRequired);

            symbols = new Kowhai.kowhai_symbol_t[] { new Kowhai.kowhai_symbol_t((UInt32)KowhaiSymbols.Symbols.Constants.Teensy),
                                                     new Kowhai.kowhai_symbol_t((UInt32)KowhaiSymbols.Symbols.Constants.TIMER2),
                                                     new Kowhai.kowhai_symbol_t((UInt32)KowhaiSymbols.Symbols.Constants.OCR2B) };
            data = new byte[] { 0 };
            if (trackBar1.Value > trackBar1.Maximum / 2)
                data[0] = (byte)((trackBar1.Value - trackBar1.Maximum / 2) * 10);
            else if (trackBar1.Value < trackBar1.Maximum / 2)
                data[0] = (byte)((Math.Abs(trackBar1.Value - trackBar1.Maximum / 2)) * 10);
            if (trackBar2.Value < trackBar2.Maximum / 2)
                data[0] = (byte)Math.Max(0, data[0] + ((trackBar2.Value - trackBar2.Maximum / 2) * 10));
            KowhaiProtocol.Create(buffer, PACKET_SIZE, ref prot, symbols, data, 0, out bytesRequired);
            SendData(buffer, bytesRequired);
        }
    }
}
