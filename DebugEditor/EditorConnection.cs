using System;
using System.Net.Sockets;
using System.IO;

namespace FalloutClient {

    class EditorConnection
    {
        private TcpClient client;
        private NetworkStream stream;
        private BinaryReader br;
        private BinaryWriter bw;

        public readonly int[] Globals;
        public readonly int[] MapVars;
        public readonly uint[,] Critters;
        public readonly ulong[] SGlobalKeys;
        public readonly int[] sGlobals;
        public readonly uint[] Arrays;
        public readonly int[] ArrayLengths;
        public readonly int[] ArrayFlag;
        public readonly bool[] ArrayIsMap;

        public EditorConnection() {
            System.Threading.Thread.Sleep(1000);
            client = new TcpClient("127.0.0.1", 4245);
            stream = client.GetStream();
            br = new BinaryReader(stream);
            bw = new BinaryWriter(stream);
            Globals = new int[br.ReadInt32()];
            MapVars = new int[br.ReadInt32()];
            SGlobalKeys = new ulong[br.ReadInt32()];
            sGlobals = new int[SGlobalKeys.Length];
            Arrays = new uint[br.ReadInt32()];
            ArrayLengths = new int[Arrays.Length];
            ArrayFlag = new int[Arrays.Length];
            ArrayIsMap = new bool[Arrays.Length];
            Critters = new uint[br.ReadInt32(), 2];

            for (int i = 0; i < Globals.Length; i++) Globals[i] = br.ReadInt32();
            for (int i = 0; i < MapVars.Length; i++) MapVars[i] = br.ReadInt32();
            for (int i = 0; i < sGlobals.Length; i++) {
                SGlobalKeys[i] = br.ReadUInt64();
                sGlobals[i] = br.ReadInt32();
                br.ReadInt32();
            }
            for (int i = 0; i < Arrays.Length; i++) {
                Arrays[i] = br.ReadUInt32();
                ArrayIsMap[i] = (br.ReadInt32() != 0);
                ArrayLengths[i] = br.ReadInt32();
                ArrayFlag[i] = br.ReadInt32();
            }
            for (int i = 0; i < Critters.Length / 2; i++) {
                Critters[i, 0] = br.ReadUInt32();
                Critters[i, 1] = br.ReadUInt32();
            }
        }

        public void Close() {
            stream.Close();
            client.Close();
            stream = null;
            client = null;
        }

        public void WriteDataType(DataTypeSend type) {
            stream.WriteByte((byte)type);
            //bw.Write((byte)type);
        }

        public void WriteByte(byte b) { bw.Write(b); }
        public void WriteInt16(short value) { bw.Write(value); }
        public void WriteInt(int value) { bw.Write(value); }
        public void WriteBytes(byte[] buf) { bw.Write(buf); }

        public void WriteBytes(byte[] buf, int start, int len) {
            bw.Write(buf, start, len);
        }

        public void Read(byte[] buffer) {
            int upto = 0;
            while (upto < buffer.Length) {
                upto += stream.Read(buffer, upto, buffer.Length - upto);
            }
        }
        public void Read(byte[] buffer, int offset, int length) {
            int upto = 0;
            //System.Diagnostics.Debug.WriteLine("Read - offset: "+offset+" length: "+length);
            while (upto < length) {
                upto += stream.Read(buffer, offset + upto, length - upto);
            }
        }
        public int ReadByte() {
            //System.Diagnostics.Debug.WriteLine("Read byte");
            return stream.ReadByte();
        }
        public int ReadShort() {
            //System.Diagnostics.Debug.WriteLine("Read short");
            return br.ReadInt16();
        }
        public int ReadInt() {
            //System.Diagnostics.Debug.WriteLine("Read int");
            return br.ReadInt32();
        }
        public uint ReadUInt() {
            //System.Diagnostics.Debug.WriteLine("Read uint");
            return br.ReadUInt32();
        }
        public byte[] ReadBytes(int count) {
            return br.ReadBytes(count);
        }
    }
}
