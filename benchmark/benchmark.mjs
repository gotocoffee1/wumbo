export const benchmark = ({ name, compile, run }) => {

    return async (data) => {
        const compileStart = performance.now();
        const result = await compile(data);
        const compileEnd = performance.now();
        await run(result);
        const runEnd = performance.now();
         
        return [name, compileEnd - compileStart, runEnd - compileEnd]
    };
};
