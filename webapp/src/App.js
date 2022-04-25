import { useEffect, useState } from "react";
import Plot from "react-plotly.js";

function App() {
  const [data, setData] = useState([]);

  async function fetchData() {
    let response = await fetch(
      "http://608dev-2.net/sandbox/sc/team10/server.py"
    );
    let body = await response.text();
    setData(JSON.parse(body));
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
          <div className="flex flex-row flex-wrap">
            <Plot
              data={[
                {
                  x: data["times"],
                  y: data["x_x"],
                  type: "scatter",
                  mode: "lines+markers",
                  marker: { color: "red" },
                },
              ]}
              layout={{ width: 320, height: 240, title: "X Position" }}
            />
            <Plot
              data={[
                {
                  x: data["times"],
                  y: data["x_y"],
                  type: "scatter",
                  mode: "lines+markers",
                  marker: { color: "red" },
                },
              ]}
              layout={{ width: 320, height: 240, title: "Y Position" }}
            />
            <Plot
              data={[
                {
                  x: data["times"],
                  y: data["v_x"],
                  type: "scatter",
                  mode: "lines+markers",
                  marker: { color: "red" },
                },
              ]}
              layout={{ width: 320, height: 240, title: "X Velocity" }}
            />
            <Plot
              data={[
                {
                  x: data["times"],
                  y: data["v_y"],
                  type: "scatter",
                  mode: "lines+markers",
                  marker: { color: "red" },
                },
              ]}
              layout={{ width: 320, height: 240, title: "Y Velocity" }}
            />
            <Plot
              data={[
                {
                  x: data["times"],
                  y: data["a_x"],
                  type: "scatter",
                  mode: "lines+markers",
                  marker: { color: "red" },
                },
              ]}
              layout={{ width: 320, height: 240, title: "X Acceleration" }}
            />
            <Plot
              data={[
                {
                  x: data["times"],
                  y: data["a_y"],
                  type: "scatter",
                  mode: "lines+markers",
                  marker: { color: "red" },
                },
              ]}
              layout={{ width: 320, height: 240, title: "Y Acceleration" }}
            />
            <Plot
              data={[
                {
                  x: data["times"],
                  y: data["speeds"],
                  type: "scatter",
                  mode: "lines+markers",
                  marker: { color: "red" },
                },
              ]}
              layout={{ width: 320, height: 240, title: "Speeds" }}
            />
            <Plot
              data={[
                {
                  x: data["times"],
                  y: data["directions"],
                  type: "scatter",
                  mode: "lines+markers",
                  marker: { color: "red" },
                },
              ]}
              layout={{ width: 320, height: 240, title: "Directions" }}
            />
          </div>
        )}
      </p>
    </div>
  );
}

export default App;
