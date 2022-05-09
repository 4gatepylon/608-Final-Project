import React from "react";

function Camera() {
  const [imgSrcList, setImgSrcList] = React.useState(["empty"]);

  React.useEffect(() => {
    async function getImgSrcList() {
      const imgSrcResult = await fetch(
        "https://608dev-2.net/sandbox/sc/team10/server.py?camera=1"
      );
      if (imgSrcResult) {
        const jsonResult = await imgSrcResult.json();
        const imgSrcString: string = jsonResult["image_decoded"];
        const newImgSrcString = [imgSrcString, imgSrcString, imgSrcString];
        setImgSrcList(newImgSrcString);
      }

      //setImgSrc(imgSrcResult);
    }
    getImgSrcList();
  }, [imgSrcList]);

  const camera = (
    <div className="relative">
      <div className="text-xl mb-5 mt-10 flex flex-row justify-center font-serif font-semibold">
        Live Feed from the camera!
      </div>
      <hr className="w-2/4 ml-auto mr-auto mb-16 text-black border-2 border-slate-800" />
      <div className="relative flex flex-row justify-center ">
        <div className="bg-white h-50 rounded-lg justify-center border-8 border-green-500">
          <img
            src={imgSrcList[0]}
            className="object-contain"
            alt={"camera output"}
          />
        </div>
      </div>
    </div>
  );

  return <div>{camera}</div>;
}

export default Camera;
