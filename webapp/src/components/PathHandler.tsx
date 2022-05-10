import React from "react";

export function PathHandler() {
    const [requestServer, setRequestServer] = React.useState(false);
    const [path, setPath] = React.useState("");
    const [time, setTime] = React.useState(0);

    const [endTime, setEndTime] = React.useState(0);

    const serverButton = (
        <button
            className={`max-w-30 mx-auto w-16 text-white font-bold py-2 px-4 rounded flex flex-row justify-center ${time===0?'bg-blue-500  hover:bg-blue-700':'bg-red-500  hover:bg-red-700'}`}
            onClick={() => {
                if (time === 0) {
                    // set time to now
                    setTime(Date.now());
                } else {
                    // store now in a new variable
                    setEndTime(Date.now());
                    setRequestServer(true);
                    setTime(0);
                }
            }
            }
        >
            {time === 0 ? "Start" : "Stop"}
        </button>
    );

    // make a textbox that asks for a string to be used as the path that will be stroed on the server
    const pathInput = (
        <input
            className="mx-auto w-64 text-indigo-800 font-bold py-2 px-4 rounded mt-8 focus:ring-blue-500"
            type="text"
            placeholder="Enter the name of the path ..."
            onChange={(e) => {
                setPath(e.target.value);
            }
            }
        />
    );
                


    return (
        <div className="inset-0 top-0 left-0 h-screen bg-cyan-100 flex flex-col">
            <div className="text-xl mx-auto mt-4 mb-4">
                Record Your Path
            </div>
            {serverButton}
            {pathInput}
            {requestServer && path !== "" && endTime !== 0 && (
                <div className="mx-auto">
                    <h1
                        className="text-4xl mx-auto mt-16 mb-4 text-green-800 font-bold"
                    >Path Name: {path}</h1>
                    {/* year, month, date and seconds for the start and the stop */}
                    <div className="flex flex-row justify-center mx-auto ">

                    <div className="text-lg mt-4 mb-4 p-2">
                    Start Time: 
                    </div>
                    <h1
                        className="text-lg mt-4 mb-4 p-2 text-green-500 font-bold"
                    >{new Date(time).toLocaleString("en-US", {
                        year: "numeric",
                        month: "long",
                        day: "numeric",
                        hour: "numeric",
                        minute: "numeric",
                        second: "numeric",
                        timeZone: "America/New_York"
                    })}</h1>
                    </div>
                    <div className="flex flex-row justify-center mx-auto ">
                    <div className="text-lg mb-4 p-2">
                    End Time: 
                    </div>
                    <h1
                        className="text-lg mb-4 p-2 text-green-500 font-bold"
                    >{new Date(endTime).toLocaleString("en-US", {
                        year: "numeric",
                        month: "long",
                        day: "numeric",
                        hour: "numeric",
                        minute: "numeric",
                        second: "numeric",
                        timeZone: "America/New_York"
                    })}</h1>
                    </div>
                </div>
            )}
        </div>
    );
  }
  
  export default PathHandler;