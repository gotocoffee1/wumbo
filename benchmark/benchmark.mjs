export const benchmark = ({ name, compile, run }) => {

    return async (data) => {
        console.log(`benchmark [${name}]`);
        console.time("compile");
        const result = await compile(data);
        console.timeEnd("compile");
        console.time("run");
        await run(result);
        console.timeEnd("run");
    };
};
