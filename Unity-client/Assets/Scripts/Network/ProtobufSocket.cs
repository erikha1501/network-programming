using System;
using System.Buffers;
using System.Net;
using System.Net.Sockets;

using Google.Protobuf;
using Unity.VisualScripting;

namespace BoomOnline2
{
    public class ProtobufSocket
    {
        public const int rawBufferSize = 1024;

        private readonly Socket socket;
        private readonly byte[] rawBuffer = new byte[rawBufferSize];

        public ProtobufSocket(IPAddress address, int port, bool blocking)
        {
            socket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
            socket.Blocking = blocking;
            Connect(address, port);
        }
            
        public void Connect(IPAddress address, int port)
        {
            socket.Connect(address, port);
        }

        public void SendProtoMessage(byte messageType, IMessage message)
        {
            int messageSize = message.CalculateSize();

            rawBuffer[0] = messageType;

            if (messageSize > 0)
            {
                message.WriteTo(rawBuffer.AsSpan().Slice(1, messageSize));
            }

            socket.Send(rawBuffer, messageSize + 1, SocketFlags.None);
        }

        public bool ReceiveProtoMessage(out byte messageType, out ReadOnlySequence<byte> messageSequence)
        {
            messageType = default;
            messageSequence = default;

            try
            {
                int read = socket.Receive(rawBuffer);

                if (read >= 1)
                {
                    messageType = rawBuffer[0];
                    messageSequence = new ReadOnlySequence<byte>(rawBuffer, 1, read - 1);

                    return true;
                }
            }
            catch (SocketException socketException)
            {
                if (socketException.SocketErrorCode != SocketError.WouldBlock)
                {
                    throw;
                }
            }

            return false;
        }
    }
}