\i setup.sql

SELECT plan(137);
SET client_min_messages TO ERROR;


SELECT has_function('pgr_dijkstratrsp',
    ARRAY['text', 'text', 'bigint', 'bigint', 'boolean','boolean']);

--SELECT todo_start();
SELECT function_returns('pgr_dijkstratrsp',
    ARRAY[ 'int', 'int', 'bigint', 'bigint', 'float', 'float' ],
    'setof record');
--SELECT todo_end();

SELECT style_dijkstra('pgr_dijkstratrsp', ', 2, 3)');
SELECT style_dijkstra('pgr_dijkstratrsp', ', 2, 3, true)');
SELECT style_dijkstra('pgr_dijkstratrsp', ', 2, 3, false)');


SELECT finish();
ROLLBACK;
