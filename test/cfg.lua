box.cfg{listen = 3301 }
box.schema.user.grant('guest', 'super')

box.execute("DROP TABLE IF EXISTS t;")
box.execute("CREATE TABLE t(id INT PRIMARY KEY, a TEXT, b DOUBLE);")
box.execute("insert into t values (1, 'tetsets', 1.123);")
box.execute("insert into t values (2, 'tetsets', 1.123);")
box.execute("insert into t values (3, 'tetsets', 1.123);")


function remote_procedure(arg1, arg2, arg3)
    return box.space.T:replace({arg1, arg2, arg3})
end

function remote_select()
    return box.space.T:select()
end
