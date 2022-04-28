import React from "react";
import Plot from "react-plotly.js";

type ServerData = {
  times: number[];
  x_x: number[];
  x_y: number[];
  v_x: number[];
  v_y: number[];
  a_x: number[];
  a_y: number[];
  speeds: number[];
  directions: number[];
};

function App() {
  let emptyData: ServerData = {
    times: [],
    x_x: [],
    x_y: [],
    v_x: [],
    v_y: [],
    a_x: [],
    a_y: [],
    speeds: [],
    directions: [],
  };

  const [data, setData] = React.useState(emptyData);

  async function fetchData() {
    let response = await fetch(
      "http://608dev-2.net/sandbox/sc/team10/server.py"
    );
    let body: string = await response.text();
    let data: ServerData = JSON.parse(body);
    setData(data);
  }

  React.useEffect(() => {
    const interval = setInterval(() => {
      fetchData();
    }, 1000);
    return () => clearInterval(interval);
  });
  return (
    <div className="p-10">
      <div className="text-xl mb-5 font-bold underline text-slate-800">
        Data
      </div>
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