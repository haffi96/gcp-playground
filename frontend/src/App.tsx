import React, { useState, useEffect } from 'react';
import io from 'socket.io-client';
import { v4 as uuidv4 } from 'uuid';

const App: React.FC = () => {
  const [socket, setSocket] = useState<Socket | null>(null);
  const [log, setLog] = useState<string[]>([]);
  const [emitData, setEmitData] = useState('');
  const [broadcastData, setBroadcastData] = useState('');
  const [joinRoom, setJoinRoom] = useState('');
  const [leaveRoom, setLeaveRoom] = useState('');
  const [roomName, setRoomName] = useState('');
  const [roomData, setRoomData] = useState('');
  const [closeRoom, setCloseRoom] = useState('');

  useEffect(() => {
    // Connect to socket server
    const newSocket = io('http://127.0.0.1:9000',
      {
        // transports: ['websocket'],
        reconnection: true,
        autoConnect: true,
        agent: false,
        auth: {
          clientId: `client_${uuidv4()}`,
          token: 'my_token'
        }
      }
    ); // Replace with your socket server URL



    newSocket.on('connect', () => {
      addToLog("Connected to server");
      addToLog("Socket ID: " + newSocket.id);
      newSocket.emit('join', { room: `room_1` });
      newSocket.emit('my_event', { data: "I'm connected!" });
    });

    newSocket.on('disconnect', () => {
      addToLog("Disconnected");
    });

    newSocket.on('my_response', (msg: { data: string }) => {
      addToLog(`Received: ${msg.data}`);
    });

    setSocket(newSocket);

    // Cleanup on component unmount
    return () => {
      newSocket.disconnect();
    };
  }, []);

  const addToLog = (message: string) => {
    setLog(prevLog => [...prevLog, message]);
  };

  const handleEmit = (e: React.FormEvent) => {
    e.preventDefault();
    socket?.emit('my_event', { data: emitData });
    setEmitData('');
  };

  const handleBroadcast = (e: React.FormEvent) => {
    e.preventDefault();
    socket?.emit('my_broadcast_event', { data: broadcastData });
    setBroadcastData('');
  };

  const handleJoinRoom = (e: React.FormEvent) => {
    e.preventDefault();
    socket?.emit('join', { room: joinRoom });
    setJoinRoom('');
  };

  const handleLeaveRoom = (e: React.FormEvent) => {
    e.preventDefault();
    socket?.emit('leave', { room: leaveRoom });
    setLeaveRoom('');
  };

  const handleSendRoom = (e: React.FormEvent) => {
    e.preventDefault();
    socket?.emit('my_room_event', { room: roomName, data: roomData });
    setRoomName('');
    setRoomData('');
  };

  const handleCloseRoom = (e: React.FormEvent) => {
    e.preventDefault();
    socket?.emit('close_room', { room: closeRoom });
    setCloseRoom('');
  };

  const handleDisconnect = (e: React.FormEvent) => {
    e.preventDefault();
    socket?.emit('disconnect_request');
  };

  return (
    <div className="p-6 max-w-xl mx-auto">
      <h1 className="text-2xl font-bold mb-4">Socket.IO Client</h1>

      <section className="mb-4">
        <h2 className="text-xl mb-2">Send:</h2>

        <form onSubmit={handleEmit} className="mb-2">
          <input
            type="text"
            value={emitData}
            onChange={(e) => setEmitData(e.target.value)}
            placeholder="Message"
            className="border p-2 mr-2"
          />
          <button type="submit" className="bg-blue-500 text-white p-2">Echo</button>
        </form>

        <form onSubmit={handleBroadcast} className="mb-2">
          <input
            type="text"
            value={broadcastData}
            onChange={(e) => setBroadcastData(e.target.value)}
            placeholder="Broadcast Message"
            className="border p-2 mr-2"
          />
          <button type="submit" className="bg-green-500 text-white p-2">Broadcast</button>
        </form>

        <form onSubmit={handleJoinRoom} className="mb-2">
          <input
            type="text"
            value={joinRoom}
            onChange={(e) => setJoinRoom(e.target.value)}
            placeholder="Room Name"
            className="border p-2 mr-2"
          />
          <button type="submit" className="bg-purple-500 text-white p-2">Join Room</button>
        </form>

        <form onSubmit={handleLeaveRoom} className="mb-2">
          <input
            type="text"
            value={leaveRoom}
            onChange={(e) => setLeaveRoom(e.target.value)}
            placeholder="Room Name"
            className="border p-2 mr-2"
          />
          <button type="submit" className="bg-red-500 text-white p-2">Leave Room</button>
        </form>

        <form onSubmit={handleSendRoom} className="mb-2 flex">
          <input
            type="text"
            value={roomName}
            onChange={(e) => setRoomName(e.target.value)}
            placeholder="Room Name"
            className="border p-2 mr-2"
          />
          <input
            type="text"
            value={roomData}
            onChange={(e) => setRoomData(e.target.value)}
            placeholder="Room Message"
            className="border p-2 mr-2"
          />
          <button type="submit" className="bg-indigo-500 text-white p-2">Send to Room</button>
        </form>

        <form onSubmit={handleCloseRoom} className="mb-2">
          <input
            type="text"
            value={closeRoom}
            onChange={(e) => setCloseRoom(e.target.value)}
            placeholder="Room Name"
            className="border p-2 mr-2"
          />
          <button type="submit" className="bg-orange-500 text-white p-2">Close Room</button>
        </form>

        <form onSubmit={handleDisconnect}>
          <button type="submit" className="bg-gray-500 text-white p-2">Disconnect</button>
        </form>
      </section>

      <section>
        <h2 className="text-xl mb-2">Receive:</h2>
        <div className="border p-4 h-48 overflow-y-auto">
          {log.map((message, index) => (
            <p key={index} className="text-sm">{message}</p>
          ))}
        </div>
      </section>
    </div>
  );
};

export default App;