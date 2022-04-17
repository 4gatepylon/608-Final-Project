import { useEffect, useState } from "react";
import Plot from "react-plotly.js";

function App() {
  const [data, setData] = useState([]);

  async function fetchData() {
    let response = await fetch(
      "http://608dev-2.net/sandbox/sc/team10/server.py"
    );
    let body = await response.text();
    console.log(JSON.parse(body));
    setData([...data, JSON.parse(body)]);
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
      <p className="text-lg">
        {data && (
          <>
            <Plot
              data={[
                {
                  x: data.times,
                  y: data.x,
                  type: "scatter",
                  mode: "lines+markers",
                  marker: { color: "red" },
                },
              ]}
              layout={{ width: 320, height: 240, title: "A Fancy Plot" }}
            />
          </>
        )}
      </p>
    </div>
  );
}

export default App;
