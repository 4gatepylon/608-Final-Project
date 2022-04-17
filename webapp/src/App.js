import { useEffect, useState } from "react";

function App() {
  const [data, setData] = useState([]);

  async function fetchData() {
    var response = await fetch(
      "http://608dev-2.net/sandbox/sc/team10/server.py"
    );
    var body = await response.text();
    setData([...data, body]);
  }

  useEffect(() => {
    const interval = setInterval(() => {
      fetchData();
    }, 1000);
    return () => clearInterval(interval);
  });

  return (
    <div className="p-10">
      <h1 className="text-lg mb-5 font-bold">Data</h1>
      <p className="text-lg">{data && <>{data}</>}</p>
    </div>
  );
}

export default App;
